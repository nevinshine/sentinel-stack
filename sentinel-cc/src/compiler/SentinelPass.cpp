#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

#include <regex>
#include <set>
#include <string>

using namespace llvm;

namespace {

// Comprehensive list of libc syscall wrappers we should track.
// These are functions that ultimately invoke 'syscall' on the user's behalf.
static bool isKnownSyscallWrapper(StringRef Name) {
  // POSIX I/O
  if (Name == "write" || Name == "read" || Name == "open" ||
      Name == "close" || Name == "lseek")
    return true;
  // GNU/glibc internal wrappers
  if (Name == "__write" || Name == "__read" || Name == "__open" ||
      Name == "__close")
    return true;
  if (Name == "__libc_write" || Name == "__libc_read" ||
      Name == "__libc_open" || Name == "__libc_close")
    return true;
  if (Name == "__sys_write" || Name == "__sys_read")
    return true;
  // Generic syscall()
  if (Name == "syscall")
    return true;
  // File operations
  if (Name == "openat" || Name == "fstat" || Name == "stat" ||
      Name == "lstat" || Name == "fstatat")
    return true;
  // Process
  if (Name == "fork" || Name == "vfork" || Name == "clone" ||
      Name == "execve" || Name == "execveat")
    return true;
  // Memory mapping
  if (Name == "mmap" || Name == "munmap" || Name == "mprotect" ||
      Name == "mremap" || Name == "brk")
    return true;
  // Network
  if (Name == "socket" || Name == "connect" || Name == "bind" ||
      Name == "listen" || Name == "accept" || Name == "accept4" ||
      Name == "send" || Name == "sendto" || Name == "recv" ||
      Name == "recvfrom" || Name == "sendmsg" || Name == "recvmsg")
    return true;
  // Dangerous
  if (Name == "ptrace" || Name == "kill" || Name == "tkill" ||
      Name == "tgkill")
    return true;
  // Signal
  if (Name == "sigaction" || Name == "rt_sigaction" ||
      Name == "rt_sigprocmask")
    return true;
  // Buffered I/O (indirectly invoke syscalls via libc)
  if (Name == "printf" || Name == "fprintf" || Name == "vprintf" ||
      Name == "vfprintf" || Name == "sprintf" || Name == "snprintf" ||
      Name == "dprintf")
    return true;
  if (Name == "puts" || Name == "fputs" || Name == "putchar" ||
      Name == "fputc" || Name == "putc")
    return true;
  if (Name == "fwrite" || Name == "fread" || Name == "fflush")
    return true;
  if (Name == "fopen" || Name == "fclose" || Name == "fdopen" ||
      Name == "freopen")
    return true;
  if (Name == "fgets" || Name == "fgetc" || Name == "getchar" ||
      Name == "getline" || Name == "getdelim")
    return true;
  if (Name == "perror")
    return true;
  return false;
}

// Phase 3: Map known wrapper to its primary x86-64 syscall number.
// Returns -1 if unknown or may invoke multiple different syscalls.
static int64_t getSyscallNumberForWrapper(StringRef Name) {
  // POSIX I/O
  if (Name == "write" || Name == "__write" || Name == "__libc_write") return 1;
  if (Name == "read" || Name == "__read" || Name == "__libc_read") return 0;
  if (Name == "open" || Name == "__open" || Name == "__libc_open") return 2;
  if (Name == "close" || Name == "__close" || Name == "__libc_close") return 3;
  if (Name == "lseek") return 8;
  if (Name == "openat") return 257;
  if (Name == "fstat") return 5;
  if (Name == "stat") return 4;
  if (Name == "lstat") return 6;
  if (Name == "fstatat") return 262;
  // Process
  if (Name == "fork") return 57;
  if (Name == "vfork") return 58;
  if (Name == "clone") return 56;
  if (Name == "execve") return 59;
  if (Name == "execveat") return 322;
  // Memory
  if (Name == "mmap") return 9;
  if (Name == "munmap") return 11;
  if (Name == "mprotect") return 10;
  if (Name == "mremap") return 25;
  if (Name == "brk") return 12;
  // Network
  if (Name == "socket") return 41;
  if (Name == "connect") return 42;
  if (Name == "bind") return 49;
  if (Name == "listen") return 50;
  if (Name == "accept") return 43;
  if (Name == "accept4") return 288;
  if (Name == "send" || Name == "sendto") return 44;
  if (Name == "recv" || Name == "recvfrom") return 45;
  if (Name == "sendmsg") return 46;
  if (Name == "recvmsg") return 47;
  // Dangerous
  if (Name == "ptrace") return 101;
  if (Name == "kill") return 62;
  if (Name == "tkill") return 200;
  if (Name == "tgkill") return 234;
  // Signal
  if (Name == "sigaction" || Name == "rt_sigaction") return 13;
  if (Name == "rt_sigprocmask") return 14;
  // Generic syscall / buffered I/O — may invoke multiple syscalls
  return -1;
}

// Phase 3: Extract syscall number from inline asm by finding the RAX
// input constraint ("a" / "{ax}" / "{rax}") and returning its constant value.
// Returns -1 if the value cannot be determined at compile time.
static int64_t extractSyscallNumberFromAsm(CallInst *CI) {
  auto *IA = cast<InlineAsm>(CI->getCalledOperand());
  auto Constraints =
      InlineAsm::ParseConstraints(IA->getConstraintString());
  unsigned argIdx = 0;
  for (const auto &C : Constraints) {
    if (C.Type == InlineAsm::isOutput) {
      if (C.isIndirect)
        argIdx++; // Indirect outputs consume an argument slot
    } else if (C.Type == InlineAsm::isInput) {
      for (const auto &Code : C.Codes) {
        if (Code == "a" || Code == "{ax}" || Code == "{rax}" ||
            Code == "{eax}") {
          if (argIdx < CI->arg_size()) {
            if (auto *CInt =
                    dyn_cast<ConstantInt>(CI->getArgOperand(argIdx)))
              return CInt->getSExtValue();
          }
          return -1; // RAX input is not a compile-time constant
        }
      }
      argIdx++;
    }
  }
  return -1; // No RAX input constraint found
}

struct SentinelPass : public PassInfoMixin<SentinelPass> {

  // Detect obfuscated syscall instructions in inline asm.
  // Catches .byte 0x0f, 0x05 and variants that bypass the "syscall" string match.
  static bool containsObfuscatedSyscall(StringRef AsmStr) {
    std::string S = AsmStr.str();
    // Pattern 1: .byte with hex 0x0f ... 0x05 (any separator)
    //   .byte 0x0f, 0x05   .byte 0xf,0x5   .byte 0x0f;.byte 0x05
    std::regex HexPat(R"(\.byte\s+0x0?[fF]\s*[,;]\s*\.?b?y?t?e?\s*0x0?5)",
                      std::regex::icase);
    if (std::regex_search(S, HexPat))
      return true;
    // Pattern 2: .byte with decimal 15, 5
    std::regex DecPat(R"(\.byte\s+15\s*[,;]\s*\.?b?y?t?e?\s*5\b)",
                      std::regex::icase);
    if (std::regex_search(S, DecPat))
      return true;
    // Pattern 3: raw escape sequences (\x0f\x05)
    if (S.find("\\x0f\\x05") != std::string::npos)
      return true;
    if (S.find("\\x0F\\x05") != std::string::npos)
      return true;
    return false;
  }

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
    LLVMContext &Ctx = M.getContext();

    std::vector<Instruction *> Syscalls;

    // 1. Identify Syscalls (Collect first to avoid iterator invalidation)
    for (Function &F : M) {
      if (F.isDeclaration())
        continue;

      for (BasicBlock &BB : F) {
        for (Instruction &I : BB) {
          bool IsSyscall = false;

          if (auto *CI = dyn_cast<CallInst>(&I)) {
            if (CI->isInlineAsm()) {
              auto *IA = cast<InlineAsm>(CI->getCalledOperand());
              StringRef AsmStr = IA->getAsmString();
              // Detect inline 'syscall' or 'int $0x80' instructions
              if (AsmStr.contains("syscall") || AsmStr.contains("int $0x80") ||
                  AsmStr.contains("int $$0x80") || AsmStr.contains("svc #0") ||
                  AsmStr.contains("svc 0")) {
                IsSyscall = true;
              }
              // Detect obfuscated syscall: .byte 0x0f, 0x05 and variants
              if (!IsSyscall && containsObfuscatedSyscall(AsmStr)) {
                errs() << "[Sentinel] WARNING: Obfuscated syscall detected in "
                       << F.getName() << " via .byte encoding!\n";
                IsSyscall = true;
              }
            } else if (Function *CalledF = CI->getCalledFunction()) {
              if (isKnownSyscallWrapper(CalledF->getName()))
                IsSyscall = true;
            }
          }

          if (IsSyscall) {
            Syscalls.push_back(&I);
          }
        }
      }
    }

    bool Modified = !Syscalls.empty();

    // 2. Instrument: Inject TCA Intrinsics
    for (Instruction *I : Syscalls) {
      BasicBlock *OldBB = I->getParent();
      Function *F = OldBB->getParent();

      // Split: NewBB starts EXACTLY at 'I' (the syscall/call instruction)
      OldBB->splitBasicBlock(I, "sentinel_site");

      // Phase 3: Extract syscall number for this site
      int64_t SyscallNr = -1;
      if (auto *CI = dyn_cast<CallInst>(I)) {
        if (CI->isInlineAsm()) {
          SyscallNr = extractSyscallNumberFromAsm(CI);
        } else if (Function *CalledF = CI->getCalledFunction()) {
          SyscallNr = getSyscallNumberForWrapper(CalledF->getName());
        }
      }

      if (SyscallNr >= 0)
        errs() << "[Sentinel] Captured syscall site in " << F->getName()
               << " (nr=" << SyscallNr << ")\n";
      else
        errs() << "[Sentinel] Captured syscall site in " << F->getName()
               << " (nr=any)\n";

      // TCA Intrinsics Injection
      IRBuilder<> B(I); // Insert exactly before the syscall instruction

      // 1. Source Capability (cs1): Cast parent function pointer to addrspace 200 (Capability)
      PointerType *CapTy = PointerType::get(Ctx, 200);
      Value *FuncPtr = F;
      Value *CapVal = B.CreateAddrSpaceCast(FuncPtr, CapTy, "tca.func.cap");

      // 2. Intent Hash (rs2): Deterministic mock hash using SyscallNr
      int64_t SafeSyscallNr = (SyscallNr >= 0) ? SyscallNr : 0;
      uint64_t IntentHash = (static_cast<uint64_t>(SafeSyscallNr) << 32) | 0x7CA;
      Value *HashVal = ConstantInt::get(Type::getInt64Ty(Ctx), IntentHash);

      // 3. Declare or get the intrinsic
      FunctionType *IntrinsicTy = FunctionType::get(CapTy, {CapTy, Type::getInt64Ty(Ctx)}, false);
      FunctionCallee TCAIntrinsic = M.getOrInsertFunction("llvm.riscv.tca.cap.setintent", IntrinsicTy);

      // 4. Inject intrinsic call
      B.CreateCall(TCAIntrinsic, {CapVal, HashVal});
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
          }};
}
