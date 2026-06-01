; ModuleID = 'tca_dummy.c'
source_filename = "tca_dummy.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-redhat-linux-gnu"

@.str = private unnamed_addr constant [25 x i8] c"TCA capability emitted\\n\00", align 1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @execute_intent() #0 {
  br label %1

1:                                                ; preds = %0
  %2 = call ptr addrspace(200) @llvm.riscv.tca.cap.setintent(ptr addrspace(200) addrspacecast (ptr @execute_intent to ptr addrspace(200)), i64 4294969290)
  %3 = call i64 @write(i32 noundef 1, ptr noundef @.str, i64 noundef 23)
  ret void
}

declare dso_local i64 @write(i32 noundef, ptr noundef, i64 noundef) #1

; Unknown intrinsic
declare ptr addrspace(200) @llvm.riscv.tca.cap.setintent(ptr addrspace(200), i64)

attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2}
!llvm.ident = !{!3}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"uwtable", i32 2}
!2 = !{i32 7, !"frame-pointer", i32 2}
!3 = !{!"clang version 21.1.8 (Fedora 21.1.8-4.fc43)"}
