; ModuleID = '../tca-prototype/ptr_test.c'
source_filename = "../tca-prototype/ptr_test.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-redhat-linux-gnu"

@__sentinel_signature = dso_local global [64 x i8] zeroinitializer, section ".signature", align 16
@.str = private unnamed_addr constant [52 x i8] c"[TCA] Sentinel Validation Environment Initialized.\0A\00", align 1
@.str.1 = private unnamed_addr constant [30 x i8] c"[TCA] Cryptographic Root: %p\0A\00", align 1
@.str.2 = private unnamed_addr constant [35 x i8] c"purpose: SpatialBufferManipulation\00", align 1
@.str.3 = private unnamed_addr constant [17 x i8] c"Buffer[%d] = %d\0A\00", align 1
@.str.4 = private unnamed_addr constant [40 x i8] c"[TCA] Intent block concluded. Exiting.\0A\00", align 1

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
  call void @llvm.telos.intent.start(ptr noundef @.str.2)
  store i32 0, ptr %3, align 4
  br label %7

7:                                                ; preds = %24, %0
  %8 = load i32, ptr %3, align 4
  %9 = icmp sle i32 %8, 10
  br i1 %9, label %10, label %27

10:                                               ; preds = %7
  %11 = load i32, ptr %3, align 4
  %12 = mul nsw i32 %11, 2
  %13 = load ptr, ptr %2, align 8
  %14 = load i32, ptr %3, align 4
  %15 = sext i32 %14 to i64
  %16 = getelementptr inbounds i32, ptr %13, i64 %15
  store i32 %12, ptr %16, align 4
  %17 = load i32, ptr %3, align 4
  %18 = load ptr, ptr %2, align 8
  %19 = load i32, ptr %3, align 4
  %20 = sext i32 %19 to i64
  %21 = getelementptr inbounds i32, ptr %18, i64 %20
  %22 = load i32, ptr %21, align 4
  %23 = call i32 (ptr, ...) @printf(ptr noundef @.str.3, i32 noundef %17, i32 noundef %22)
  br label %24

24:                                               ; preds = %10
  %25 = load i32, ptr %3, align 4
  %26 = add nsw i32 %25, 1
  store i32 %26, ptr %3, align 4
  br label %7, !llvm.loop !6

27:                                               ; preds = %7
  call void @llvm.telos.intent.end()
  %28 = load ptr, ptr %2, align 8
  call void @free(ptr noundef %28) #5
  %29 = call i32 (ptr, ...) @printf(ptr noundef @.str.4)
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
