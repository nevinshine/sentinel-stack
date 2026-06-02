; ModuleID = 'tests/mock_intent.c'
source_filename = "tests/mock_intent.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-redhat-linux-gnu"

@.tca_got_table = constant [1 x i64] [i64 -8526495043115022390], section ".tca_got", align 8

; Function Attrs: nounwind uwtable
define dso_local void @safe_logger() #0 {
  %1 = load volatile i64, ptr @.tca_got_table, align 8
  %2 = tail call ptr addrspace(200) @llvm.riscv.tca.cap.setintent(ptr addrspace(200) addrspacecast (ptr @safe_logger to ptr addrspace(200)), i64 %1) #1
  tail call void @llvm.riscv.tca.cap.clear() #1
  ret void
}

; Function Attrs: nounwind uwtable
define dso_local noundef i32 @main() local_unnamed_addr #0 {
  %1 = load volatile i64, ptr @.tca_got_table, align 8
  %2 = tail call ptr addrspace(200) @llvm.riscv.tca.cap.setintent(ptr addrspace(200) addrspacecast (ptr @safe_logger to ptr addrspace(200)), i64 %1) #1
  tail call void @llvm.riscv.tca.cap.clear() #1
  ret i32 0
}

; Unknown intrinsic
declare ptr addrspace(200) @llvm.riscv.tca.cap.setintent(ptr addrspace(200), i64)

; Unknown intrinsic
declare void @llvm.riscv.tca.cap.clear()

attributes #0 = { nounwind uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nounwind }

!llvm.module.flags = !{!0, !1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"uwtable", i32 2}
!2 = !{!"clang version 21.1.8 (Fedora 21.1.8-4.fc43)"}
