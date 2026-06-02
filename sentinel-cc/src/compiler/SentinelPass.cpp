#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/CRC.h"
#include "llvm/Support/raw_ostream.h"

#include <set>
#include <string>
#include <vector>

using namespace llvm;

namespace {

struct SentinelPass : public PassInfoMixin<SentinelPass> {

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
    LLVMContext &Ctx = M.getContext();
    bool Modified = false;

    // Collect all intent.start calls
    struct IntentInfo {
      CallInst *StartInst;
      uint32_t Hash;
    };
    std::vector<IntentInfo> Intents;

    for (Function &F : M) {
      if (F.isDeclaration())
        continue;

      for (BasicBlock &BB : F) {
        for (Instruction &I : BB) {
          if (auto *CI = dyn_cast<CallInst>(&I)) {
            if (Function *CalledF = CI->getCalledFunction()) {
              if (CalledF->getName() == "llvm.telos.intent.start") {
                // Compute deterministic hash (Z3 Proof ID placeholder)
                // In a production compiler, this would hash the string argument or enum.
                // Using a deterministic hash here for the prototype.
                uint32_t Z3Hash = 0x89ABCDEF; 
                Intents.push_back({CI, Z3Hash});
              }
            }
          }
        }
      }
    }

    if (Intents.empty())
      return PreservedAnalyses::all();

    Modified = true;

    // 1. Create .tca_got GlobalVariable Array (8-byte aligned)
    Type *Int64Ty = Type::getInt64Ty(Ctx);
    ArrayType *GotArrayTy = ArrayType::get(Int64Ty, Intents.size());
    std::vector<Constant *> GotValues;

    for (size_t i = 0; i < Intents.size(); ++i) {
      // Pack the Z3 Hash [63:32] with the PT_SHADOW_PROOF marker/index [31:0]
      uint64_t Packed = ((uint64_t)Intents[i].Hash << 32) | 0x7CA;
      GotValues.push_back(ConstantInt::get(Int64Ty, Packed));
    }

    Constant *GotInit = ConstantArray::get(GotArrayTy, GotValues);
    GlobalVariable *TcaGot = new GlobalVariable(
        M, GotArrayTy, true, GlobalValue::ExternalLinkage, GotInit, ".tca_got_table");
    TcaGot->setSection(".tca_got");
    TcaGot->setAlignment(Align(8));

    // Get Intrinsics
    PointerType *CapTy = PointerType::get(Ctx, 200);
    FunctionType *SetIntentTy =
        FunctionType::get(CapTy, {CapTy, Int64Ty}, false);
    FunctionCallee SetIntrinsic =
        M.getOrInsertFunction("tca_set_intent", SetIntentTy);

    FunctionType *ClearTy = FunctionType::get(Type::getVoidTy(Ctx), {}, false);
    FunctionCallee ClearIntrinsic =
        M.getOrInsertFunction("tca_clear", ClearTy);

    FunctionAnalysisManager &FAM =
        AM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();

    // 2. Inject csetintent and cclear using DominatorTree Analysis
    for (size_t i = 0; i < Intents.size(); ++i) {
      CallInst *StartInst = Intents[i].StartInst;
      Function *F = StartInst->getFunction();

      DominatorTree &DT = FAM.getResult<DominatorTreeAnalysis>(*F);
      DominanceFrontier &DF = FAM.getResult<DominanceFrontierAnalysis>(*F);

      BasicBlock *StartBB = StartInst->getParent();

      // Inject csetintent right before the start instruction
      IRBuilder<> B(StartInst);
      Value *Zero = ConstantInt::get(Type::getInt32Ty(Ctx), 0);
      Value *Idx = ConstantInt::get(Type::getInt32Ty(Ctx), i);
      Value *Gep = B.CreateGEP(GotArrayTy, TcaGot, {Zero, Idx}, "tca_got_gep");
      LoadInst *LoadedHash = B.CreateLoad(Int64Ty, Gep, "tca_got_load");
      LoadedHash->setVolatile(true); // Prevent LLVM from folding this load at compile time

      Value *FuncPtr = F;
      Value *CapVal = B.CreateAddrSpaceCast(FuncPtr, CapTy, "tca.func.cap");
      B.CreateCall(SetIntrinsic, {CapVal, LoadedHash});

      // Find all exit points from the intent scope
      std::set<Instruction *> ExitPoints;

      // a) Any block in the dominance frontier of the StartBB
      auto it = DF.find(StartBB);
      if (it != DF.end()) {
        for (BasicBlock *FrontierBB : it->second) {
          ExitPoints.insert(&FrontierBB->front());
        }
      }

      // b) Any return/resume instruction in blocks dominated by the StartBB
      for (auto *Node : depth_first(DT.getNode(StartBB))) {
        BasicBlock *DomBB = Node->getBlock();
        Instruction *Term = DomBB->getTerminator();
        if (isa<ReturnInst>(Term) || isa<ResumeInst>(Term)) {
          ExitPoints.insert(Term);
        }
      }

      // Inject cclear at all calculated exit points
      for (Instruction *ExitInst : ExitPoints) {
        IRBuilder<> ExitB(ExitInst);
        ExitB.CreateCall(ClearIntrinsic);
      }

      StartInst->eraseFromParent();
    }

    // Clean up explicit .end calls if the frontend emitted them directly
    for (Function &F : M) {
      if (F.isDeclaration())
        continue;
      std::vector<Instruction *> ToDelete;
      for (BasicBlock &BB : F) {
        for (Instruction &I : BB) {
          if (auto *CI = dyn_cast<CallInst>(&I)) {
            if (Function *CalledF = CI->getCalledFunction()) {
              if (CalledF->getName() == "llvm.telos.intent.end") {
                IRBuilder<> B(CI);
                B.CreateCall(ClearIntrinsic);
                ToDelete.push_back(CI);
              }
            }
          }
        }
      }
      for (auto *I : ToDelete)
        I->eraseFromParent();
    }

    return Modified ? PreservedAnalyses::none() : PreservedAnalyses::all();
  }
};

} // namespace

// Registration
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "SentinelPass", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level) {
                  MPM.addPass(SentinelPass());
                });
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "sentinel-tca") {
                    MPM.addPass(SentinelPass());
                    return true;
                  }
                  return false;
                });
          }};
}
