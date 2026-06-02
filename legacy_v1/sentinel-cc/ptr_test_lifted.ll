; ModuleID = 'ptr_test.ll'
source_filename = "../tca-prototype/ptr_test.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-redhat-linux-gnu"

@__sentinel_signature = dso_local global [64 x i8] zeroinitializer, section ".signature", align 16
@.str = private unnamed_addr constant [52 x i8] c"[TCA] Sentinel Validation Environment Initialized.\0A\00", align 1
@.str.1 = private unnamed_addr constant [30 x i8] c"[TCA] Cryptographic Root: %p\0A\00", align 1
@.str.2 = private unnamed_addr constant [35 x i8] c"purpose: SpatialBufferManipulation\00", align 1
@.str.3 = private unnamed_addr constant [17 x i8] c"Buffer[%d] = %d\0A\00", align 1
@.str.4 = private unnamed_addr constant [40 x i8] c"[TCA] Intent block concluded. Exiting.\0A\00", align 1
@.tca_got_table = constant [1 x i64] [i64 -8526495043115022390], section ".tca_got", align 8

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 {
  %1 = alloca i32, align 4
  %2 = alloca ptr, align 8
  %3 = alloca i32, align 4
  store i32 0, ptr %1, align 4
  %4 = call i32 (ptr, ...) @printf(ptr noundef @.str)
  %5 = call i32 (ptr, ...) @printf(ptr noundef @.str.1, ptr noundef @__sentinel_signature)
  %6 = call noalias ptr @malloc(i64 noundef 40) #4
  store ptr %6, ptr %2, align 8
  %tca_got_load = load volatile i64, ptr @.tca_got_table, align 8
  %7 = call ptr addrspace(200) @llvm.riscv.tca.cap.setintent(ptr addrspace(200) addrspacecast (ptr @main to ptr addrspace(200)), i64 %tca_got_load)
  store i32 0, ptr %3, align 4
  br label %8

8:                                                ; preds = %25, %0
  %9 = load i32, ptr %3, align 4
  %10 = icmp sle i32 %9, 10
  br i1 %10, label %11, label %28

11:                                               ; preds = %8
  %12 = load i32, ptr %3, align 4
  %13 = mul nsw i32 %12, 2
  %14 = load ptr, ptr %2, align 8
  %15 = load i32, ptr %3, align 4
  %16 = sext i32 %15 to i64
  %17 = getelementptr inbounds i32, ptr %14, i64 %16
  store i32 %13, ptr %17, align 4
  %18 = load i32, ptr %3, align 4
  %19 = load ptr, ptr %2, align 8
  %20 = load i32, ptr %3, align 4
  %21 = sext i32 %20 to i64
  %22 = getelementptr inbounds i32, ptr %19, i64 %21
  %23 = load i32, ptr %22, align 4
  %24 = call i32 (ptr, ...) @printf(ptr noundef @.str.3, i32 noundef %18, i32 noundef %23)
  br label %25

25:                                               ; preds = %11
  %26 = load i32, ptr %3, align 4
  %27 = add nsw i32 %26, 1
  store i32 %27, ptr %3, align 4
  br label %8, !llvm.loop !6

28:                                               ; preds = %8
  call void @llvm.riscv.tca.cap.clear()
  %29 = load ptr, ptr %2, align 8
  call void @free(ptr noundef %29) #5
  %30 = call i32 (ptr, ...) @printf(ptr noundef @.str.4)
  call void @llvm.riscv.tca.cap.clear()
  ret i32 0
}

declare i32 @printf(ptr noundef, ...) #1

; Function Attrs: nounwind allocsize(0)
declare noalias ptr @malloc(i64 noundef) #2

; Unknown intrinsic
declare void @llvm.telos.intent.start(ptr noundef) #1

; Unknown intrinsic
declare void @llvm.telos.intent.end() #1

; Function Attrs: nounwind
declare void @free(ptr noundef) #3

; Unknown intrinsic
declare ptr addrspace(200) @llvm.riscv.tca.cap.setintent(ptr addrspace(200), i64)

; Unknown intrinsic
declare void @llvm.riscv.tca.cap.clear()

attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { nounwind allocsize(0) "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #3 = { nounwind "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #4 = { nounwind allocsize(0) }
attributes #5 = { nounwind }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"clang version 21.1.8 (Fedora 21.1.8-4.fc43)"}
!6 = distinct !{!6, !7}
!7 = !{!"llvm.loop.mustprogress"}
