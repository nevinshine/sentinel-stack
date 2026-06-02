; ModuleID = '../tca-prototype/benchmark.c'
source_filename = "../tca-prototype/benchmark.c"
target datalayout = "e-m:e-p:64:64-i64:64-i128:128-n32:64-S128"
target triple = "riscv64-unknown-unknown-elf"

@__sentinel_signature = dso_local global [64 x i8] zeroinitializer, section ".signature", align 1
@.str = private unnamed_addr constant [23 x i8] c"[BENCHMARK] Complete.\0A\00", align 1
@.str.1 = private unnamed_addr constant [42 x i8] c"========================================\0A\00", align 1
@.str.2 = private unnamed_addr constant [41 x i8] c"[TCA] Teleological Capability Benchmark\0A\00", align 1
@.str.3 = private unnamed_addr constant [25 x i8] c"network_send_socket_data\00", align 1
@.str.4 = private unnamed_addr constant [29 x i8] c"[DATA] BASELINE_LOAD_CYCLES=\00", align 1
@.str.5 = private unnamed_addr constant [2 x i8] c"\0A\00", align 1
@.str.6 = private unnamed_addr constant [32 x i8] c"[DATA] TCA_NATIVE_STORE_CYCLES=\00", align 1
@.str.7 = private unnamed_addr constant [36 x i8] c"[DATA] EBPF_SIMULATED_STORE_CYCLES=\00", align 1
@.str.8 = private unnamed_addr constant [33 x i8] c"[DATA] TCA_OVERHEAD_VS_BASELINE=\00", align 1
@.str.9 = private unnamed_addr constant [34 x i8] c"[DATA] EBPF_OVERHEAD_VS_BASELINE=\00", align 1

; Function Attrs: noinline nounwind optnone
define dso_local void @tca_set_intent(ptr noundef %0, i64 noundef %1) #0 {
  %3 = alloca ptr, align 8
  %4 = alloca i64, align 8
  store ptr %0, ptr %3, align 8
  store i64 %1, ptr %4, align 8
  %5 = load ptr, ptr %3, align 8
  %6 = load i64, ptr %4, align 8
  call void asm sideeffect ".insn r 0x0B, 0x0, 0x01, x0, $0, $1", "r,r"(ptr %5, i64 %6) #2, !srcloc !10
  ret void
}

; Function Attrs: noinline nounwind optnone
define dso_local void @tca_clear() #0 {
  call void asm sideeffect ".insn r 0x0B, 0x0, 0x02, x0, x0, x0", ""() #2, !srcloc !11
  ret void
}

; Function Attrs: noinline nounwind optnone
define dso_local void @simulate_ebpf_lsm_hook() #0 {
  %1 = alloca [4 x i64], align 8
  %2 = alloca i64, align 8
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i64, align 8
  %6 = alloca i64, align 8
  %7 = alloca i64, align 8
  %8 = alloca i64, align 8
  %9 = getelementptr inbounds [4 x i64], ptr %1, i64 0, i64 0
  store volatile i64 160, ptr %9, align 8
  %10 = getelementptr inbounds [4 x i64], ptr %1, i64 0, i64 1
  store volatile i64 161, ptr %10, align 8
  %11 = getelementptr inbounds [4 x i64], ptr %1, i64 0, i64 2
  store volatile i64 162, ptr %11, align 8
  %12 = getelementptr inbounds [4 x i64], ptr %1, i64 0, i64 3
  store volatile i64 163, ptr %12, align 8
  store volatile i64 0, ptr %2, align 8
  store i32 0, ptr %3, align 4
  br label %13

13:                                               ; preds = %20, %0
  %14 = load i32, ptr %3, align 4
  %15 = icmp slt i32 %14, 20
  br i1 %15, label %16, label %23

16:                                               ; preds = %13
  %17 = load volatile i64, ptr %2, align 8
  %18 = mul i64 %17, 6364136223846793005
  %19 = add i64 %18, 1442695040888963407
  store volatile i64 %19, ptr %2, align 8
  br label %20

20:                                               ; preds = %16
  %21 = load i32, ptr %3, align 4
  %22 = add nsw i32 %21, 1
  store i32 %22, ptr %3, align 4
  br label %13, !llvm.loop !12

23:                                               ; preds = %13
  %24 = load volatile i64, ptr %2, align 8
  %25 = and i64 %24, 1
  %26 = icmp ne i64 %25, 0
  %27 = zext i1 %26 to i64
  %28 = select i1 %26, i32 1, i32 0
  store volatile i32 %28, ptr %4, align 4
  %29 = load volatile i32, ptr %4, align 4
  %30 = getelementptr inbounds [4 x i64], ptr %1, i64 0, i64 0
  %31 = load volatile i64, ptr %30, align 8
  store volatile i64 %31, ptr %5, align 8
  %32 = load volatile i64, ptr %5, align 8
  %33 = getelementptr inbounds [4 x i64], ptr %1, i64 0, i64 1
  %34 = load volatile i64, ptr %33, align 8
  store volatile i64 %34, ptr %6, align 8
  %35 = load volatile i64, ptr %6, align 8
  %36 = getelementptr inbounds [4 x i64], ptr %1, i64 0, i64 2
  %37 = load volatile i64, ptr %36, align 8
  store volatile i64 %37, ptr %7, align 8
  %38 = load volatile i64, ptr %7, align 8
  %39 = getelementptr inbounds [4 x i64], ptr %1, i64 0, i64 3
  %40 = load volatile i64, ptr %39, align 8
  store volatile i64 %40, ptr %8, align 8
  %41 = load volatile i64, ptr %8, align 8
  ret void
}

; Function Attrs: noinline nounwind optnone
define dso_local void @_start() #0 {
  %1 = call signext i32 @main() #3
  call void @uart_puts(ptr noundef @.str) #3
  br label %2

2:                                                ; preds = %0, %2
  call void asm sideeffect "wfi", ""() #2, !srcloc !14
  br label %2
}

; Function Attrs: noinline nounwind optnone
define dso_local signext i32 @main() #0 {
  %1 = alloca ptr, align 8
  %2 = alloca ptr, align 8
  %3 = alloca ptr, align 8
  %4 = alloca i64, align 8
  %5 = alloca i64, align 8
  %6 = alloca i64, align 8
  %7 = alloca i64, align 8
  %8 = alloca i64, align 8
  %9 = alloca i64, align 8
  %10 = alloca i64, align 8
  %11 = alloca i64, align 8
  %12 = alloca i64, align 8
  %13 = alloca i64, align 8
  store ptr @__sentinel_signature, ptr %1, align 8
  %14 = load ptr, ptr %1, align 8
  call void @uart_puts(ptr noundef @.str.1) #3
  call void @uart_puts(ptr noundef @.str.2) #3
  call void @uart_puts(ptr noundef @.str.1) #3
  store ptr inttoptr (i64 2280652800 to ptr), ptr %2, align 8
  store ptr inttoptr (i64 2248146944 to ptr), ptr %3, align 8
  %15 = call i64 @rdcycle() #3
  store i64 %15, ptr %4, align 8
  %16 = load ptr, ptr %2, align 8
  %17 = load volatile i64, ptr %16, align 8
  store volatile i64 %17, ptr %5, align 8
  %18 = call i64 @rdcycle() #3
  store i64 %18, ptr %6, align 8
  %19 = load volatile i64, ptr %5, align 8
  %20 = load i64, ptr %6, align 8
  %21 = load i64, ptr %4, align 8
  %22 = sub i64 %20, %21
  store i64 %22, ptr %7, align 8
  call void @llvm.telos.intent.start(ptr noundef @.str.3) #3
  %23 = call i64 @rdcycle() #3
  store i64 %23, ptr %8, align 8
  %24 = load ptr, ptr %2, align 8
  store volatile i64 -2401053089206453570, ptr %24, align 8
  %25 = call i64 @rdcycle() #3
  store i64 %25, ptr %9, align 8
  %26 = load i64, ptr %9, align 8
  %27 = load i64, ptr %8, align 8
  %28 = sub i64 %26, %27
  store i64 %28, ptr %10, align 8
  call void @llvm.telos.intent.end() #3
  %29 = call i64 @rdcycle() #3
  store i64 %29, ptr %11, align 8
  call void @simulate_ebpf_lsm_hook() #3
  %30 = load ptr, ptr %3, align 8
  store volatile i64 -3819410105021120785, ptr %30, align 8
  %31 = call i64 @rdcycle() #3
  store i64 %31, ptr %12, align 8
  %32 = load i64, ptr %12, align 8
  %33 = load i64, ptr %11, align 8
  %34 = sub i64 %32, %33
  store i64 %34, ptr %13, align 8
  call void @uart_puts(ptr noundef @.str.4) #3
  %35 = load i64, ptr %7, align 8
  call void @uart_u64(i64 noundef %35) #3
  call void @uart_puts(ptr noundef @.str.5) #3
  call void @uart_puts(ptr noundef @.str.6) #3
  %36 = load i64, ptr %10, align 8
  call void @uart_u64(i64 noundef %36) #3
  call void @uart_puts(ptr noundef @.str.5) #3
  call void @uart_puts(ptr noundef @.str.7) #3
  %37 = load i64, ptr %13, align 8
  call void @uart_u64(i64 noundef %37) #3
  call void @uart_puts(ptr noundef @.str.5) #3
  call void @uart_puts(ptr noundef @.str.8) #3
  %38 = load i64, ptr %10, align 8
  %39 = load i64, ptr %7, align 8
  %40 = icmp uge i64 %38, %39
  br i1 %40, label %41, label %45

41:                                               ; preds = %0
  %42 = load i64, ptr %10, align 8
  %43 = load i64, ptr %7, align 8
  %44 = sub i64 %42, %43
  call void @uart_u64(i64 noundef %44) #3
  br label %46

45:                                               ; preds = %0
  call void @uart_u64(i64 noundef 0) #3
  br label %46

46:                                               ; preds = %45, %41
  call void @uart_puts(ptr noundef @.str.5) #3
  call void @uart_puts(ptr noundef @.str.9) #3
  %47 = load i64, ptr %13, align 8
  %48 = load i64, ptr %7, align 8
  %49 = icmp uge i64 %47, %48
  br i1 %49, label %50, label %54

50:                                               ; preds = %46
  %51 = load i64, ptr %13, align 8
  %52 = load i64, ptr %7, align 8
  %53 = sub i64 %51, %52
  call void @uart_u64(i64 noundef %53) #3
  br label %55

54:                                               ; preds = %46
  call void @uart_u64(i64 noundef 0) #3
  br label %55

55:                                               ; preds = %54, %50
  call void @uart_puts(ptr noundef @.str.5) #3
  ret i32 0
}

; Function Attrs: noinline nounwind optnone
define internal void @uart_puts(ptr noundef %0) #0 {
  %2 = alloca ptr, align 8
  store ptr %0, ptr %2, align 8
  br label %3

3:                                                ; preds = %7, %1
  %4 = load ptr, ptr %2, align 8
  %5 = load i8, ptr %4, align 1
  %6 = icmp ne i8 %5, 0
  br i1 %6, label %7, label %11

7:                                                ; preds = %3
  %8 = load ptr, ptr %2, align 8
  %9 = getelementptr inbounds nuw i8, ptr %8, i32 1
  store ptr %9, ptr %2, align 8
  %10 = load i8, ptr %8, align 1
  call void @uart_putc(i8 noundef zeroext %10) #3
  br label %3, !llvm.loop !15

11:                                               ; preds = %3
  ret void
}

; Function Attrs: noinline nounwind optnone
define internal i64 @rdcycle() #0 {
  %1 = alloca i64, align 8
  %2 = call i64 asm sideeffect "csrr $0, mcycle", "=r"() #2, !srcloc !16
  store i64 %2, ptr %1, align 8
  %3 = load i64, ptr %1, align 8
  ret i64 %3
}

; Unknown intrinsic
declare void @llvm.telos.intent.start(ptr noundef) #1

; Unknown intrinsic
declare void @llvm.telos.intent.end() #1

; Function Attrs: noinline nounwind optnone
define internal void @uart_u64(i64 noundef %0) #0 {
  %2 = alloca i64, align 8
  %3 = alloca [24 x i8], align 1
  %4 = alloca i32, align 4
  store i64 %0, ptr %2, align 8
  store i32 0, ptr %4, align 4
  %5 = load i64, ptr %2, align 8
  %6 = icmp eq i64 %5, 0
  br i1 %6, label %7, label %8

7:                                                ; preds = %1
  call void @uart_putc(i8 noundef zeroext 48) #3
  br label %33

8:                                                ; preds = %1
  br label %9

9:                                                ; preds = %12, %8
  %10 = load i64, ptr %2, align 8
  %11 = icmp ne i64 %10, 0
  br i1 %11, label %12, label %23

12:                                               ; preds = %9
  %13 = load i64, ptr %2, align 8
  %14 = urem i64 %13, 10
  %15 = add i64 48, %14
  %16 = trunc i64 %15 to i8
  %17 = load i32, ptr %4, align 4
  %18 = add nsw i32 %17, 1
  store i32 %18, ptr %4, align 4
  %19 = sext i32 %17 to i64
  %20 = getelementptr inbounds [24 x i8], ptr %3, i64 0, i64 %19
  store i8 %16, ptr %20, align 1
  %21 = load i64, ptr %2, align 8
  %22 = udiv i64 %21, 10
  store i64 %22, ptr %2, align 8
  br label %9, !llvm.loop !17

23:                                               ; preds = %9
  br label %24

24:                                               ; preds = %27, %23
  %25 = load i32, ptr %4, align 4
  %26 = icmp ne i32 %25, 0
  br i1 %26, label %27, label %33

27:                                               ; preds = %24
  %28 = load i32, ptr %4, align 4
  %29 = add nsw i32 %28, -1
  store i32 %29, ptr %4, align 4
  %30 = sext i32 %29 to i64
  %31 = getelementptr inbounds [24 x i8], ptr %3, i64 0, i64 %30
  %32 = load i8, ptr %31, align 1
  call void @uart_putc(i8 noundef zeroext %32) #3
  br label %24, !llvm.loop !18

33:                                               ; preds = %7, %24
  ret void
}

; Function Attrs: noinline nounwind optnone
define internal void @uart_putc(i8 noundef zeroext %0) #0 {
  %2 = alloca i8, align 1
  store i8 %0, ptr %2, align 1
  %3 = load i8, ptr %2, align 1
  store volatile i8 %3, ptr inttoptr (i64 268435456 to ptr), align 1
  ret void
}

attributes #0 = { noinline nounwind optnone "frame-pointer"="all" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="generic-rv64" "target-features"="+64bit,+a,+c,+d,+f,+m,+relax,+zaamo,+zalrsc,+zca,+zcd,+zicsr,+zifencei,+zmmul,-b,-e,-experimental-p,-experimental-smctr,-experimental-ssctr,-experimental-svukte,-experimental-xqccmp,-experimental-xqcia,-experimental-xqciac,-experimental-xqcibi,-experimental-xqcibm,-experimental-xqcicli,-experimental-xqcicm,-experimental-xqcics,-experimental-xqcicsr,-experimental-xqciint,-experimental-xqciio,-experimental-xqcilb,-experimental-xqcili,-experimental-xqcilia,-experimental-xqcilo,-experimental-xqcilsm,-experimental-xqcisim,-experimental-xqcisls,-experimental-xqcisync,-experimental-xrivosvisni,-experimental-xrivosvizip,-experimental-xsfmclic,-experimental-xsfsclic,-experimental-zalasr,-experimental-zicfilp,-experimental-zicfiss,-experimental-zvbc32e,-experimental-zvkgs,-experimental-zvqdotq,-h,-q,-sdext,-sdtrig,-sha,-shcounterenw,-shgatpa,-shlcofideleg,-shtvala,-shvsatpa,-shvstvala,-shvstvecd,-smaia,-smcdeleg,-smcntrpmf,-smcsrind,-smdbltrp,-smepmp,-smmpm,-smnpm,-smrnmi,-smstateen,-ssaia,-ssccfg,-ssccptr,-sscofpmf,-sscounterenw,-sscsrind,-ssdbltrp,-ssnpm,-sspm,-ssqosid,-ssstateen,-ssstrict,-sstc,-sstvala,-sstvecd,-ssu64xl,-supm,-svade,-svadu,-svbare,-svinval,-svnapot,-svpbmt,-svvptc,-v,-xandesbfhcvt,-xandesperf,-xandesvbfhcvt,-xandesvdot,-xandesvpackfph,-xandesvsintload,-xcvalu,-xcvbi,-xcvbitmanip,-xcvelw,-xcvmac,-xcvmem,-xcvsimd,-xmipscbop,-xmipscmov,-xmipslsp,-xsfcease,-xsfmm128t,-xsfmm16t,-xsfmm32a16f,-xsfmm32a32f,-xsfmm32a8f,-xsfmm32a8i,-xsfmm32t,-xsfmm64a64f,-xsfmm64t,-xsfmmbase,-xsfvcp,-xsfvfnrclipxfqf,-xsfvfwmaccqqq,-xsfvqmaccdod,-xsfvqmaccqoq,-xsifivecdiscarddlone,-xsifivecflushdlone,-xtheadba,-xtheadbb,-xtheadbs,-xtheadcmo,-xtheadcondmov,-xtheadfmemidx,-xtheadmac,-xtheadmemidx,-xtheadmempair,-xtheadsync,-xtheadvdot,-xventanacondops,-xwchc,-za128rs,-za64rs,-zabha,-zacas,-zama16b,-zawrs,-zba,-zbb,-zbc,-zbkb,-zbkc,-zbkx,-zbs,-zcb,-zce,-zcf,-zclsd,-zcmop,-zcmp,-zcmt,-zdinx,-zfa,-zfbfmin,-zfh,-zfhmin,-zfinx,-zhinx,-zhinxmin,-zic64b,-zicbom,-zicbop,-zicboz,-ziccamoa,-ziccamoc,-ziccif,-zicclsm,-ziccrse,-zicntr,-zicond,-zihintntl,-zihintpause,-zihpm,-zilsd,-zimop,-zk,-zkn,-zknd,-zkne,-zknh,-zkr,-zks,-zksed,-zksh,-zkt,-ztso,-zvbb,-zvbc,-zve32f,-zve32x,-zve64d,-zve64f,-zve64x,-zvfbfmin,-zvfbfwma,-zvfh,-zvfhmin,-zvkb,-zvkg,-zvkn,-zvknc,-zvkned,-zvkng,-zvknha,-zvknhb,-zvks,-zvksc,-zvksed,-zvksg,-zvksh,-zvkt,-zvl1024b,-zvl128b,-zvl16384b,-zvl2048b,-zvl256b,-zvl32768b,-zvl32b,-zvl4096b,-zvl512b,-zvl64b,-zvl65536b,-zvl8192b" }
attributes #1 = { "frame-pointer"="all" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="generic-rv64" "target-features"="+64bit,+a,+c,+d,+f,+m,+relax,+zaamo,+zalrsc,+zca,+zcd,+zicsr,+zifencei,+zmmul,-b,-e,-experimental-p,-experimental-smctr,-experimental-ssctr,-experimental-svukte,-experimental-xqccmp,-experimental-xqcia,-experimental-xqciac,-experimental-xqcibi,-experimental-xqcibm,-experimental-xqcicli,-experimental-xqcicm,-experimental-xqcics,-experimental-xqcicsr,-experimental-xqciint,-experimental-xqciio,-experimental-xqcilb,-experimental-xqcili,-experimental-xqcilia,-experimental-xqcilo,-experimental-xqcilsm,-experimental-xqcisim,-experimental-xqcisls,-experimental-xqcisync,-experimental-xrivosvisni,-experimental-xrivosvizip,-experimental-xsfmclic,-experimental-xsfsclic,-experimental-zalasr,-experimental-zicfilp,-experimental-zicfiss,-experimental-zvbc32e,-experimental-zvkgs,-experimental-zvqdotq,-h,-q,-sdext,-sdtrig,-sha,-shcounterenw,-shgatpa,-shlcofideleg,-shtvala,-shvsatpa,-shvstvala,-shvstvecd,-smaia,-smcdeleg,-smcntrpmf,-smcsrind,-smdbltrp,-smepmp,-smmpm,-smnpm,-smrnmi,-smstateen,-ssaia,-ssccfg,-ssccptr,-sscofpmf,-sscounterenw,-sscsrind,-ssdbltrp,-ssnpm,-sspm,-ssqosid,-ssstateen,-ssstrict,-sstc,-sstvala,-sstvecd,-ssu64xl,-supm,-svade,-svadu,-svbare,-svinval,-svnapot,-svpbmt,-svvptc,-v,-xandesbfhcvt,-xandesperf,-xandesvbfhcvt,-xandesvdot,-xandesvpackfph,-xandesvsintload,-xcvalu,-xcvbi,-xcvbitmanip,-xcvelw,-xcvmac,-xcvmem,-xcvsimd,-xmipscbop,-xmipscmov,-xmipslsp,-xsfcease,-xsfmm128t,-xsfmm16t,-xsfmm32a16f,-xsfmm32a32f,-xsfmm32a8f,-xsfmm32a8i,-xsfmm32t,-xsfmm64a64f,-xsfmm64t,-xsfmmbase,-xsfvcp,-xsfvfnrclipxfqf,-xsfvfwmaccqqq,-xsfvqmaccdod,-xsfvqmaccqoq,-xsifivecdiscarddlone,-xsifivecflushdlone,-xtheadba,-xtheadbb,-xtheadbs,-xtheadcmo,-xtheadcondmov,-xtheadfmemidx,-xtheadmac,-xtheadmemidx,-xtheadmempair,-xtheadsync,-xtheadvdot,-xventanacondops,-xwchc,-za128rs,-za64rs,-zabha,-zacas,-zama16b,-zawrs,-zba,-zbb,-zbc,-zbkb,-zbkc,-zbkx,-zbs,-zcb,-zce,-zcf,-zclsd,-zcmop,-zcmp,-zcmt,-zdinx,-zfa,-zfbfmin,-zfh,-zfhmin,-zfinx,-zhinx,-zhinxmin,-zic64b,-zicbom,-zicbop,-zicboz,-ziccamoa,-ziccamoc,-ziccif,-zicclsm,-ziccrse,-zicntr,-zicond,-zihintntl,-zihintpause,-zihpm,-zilsd,-zimop,-zk,-zkn,-zknd,-zkne,-zknh,-zkr,-zks,-zksed,-zksh,-zkt,-ztso,-zvbb,-zvbc,-zve32f,-zve32x,-zve64d,-zve64f,-zve64x,-zvfbfmin,-zvfbfwma,-zvfh,-zvfhmin,-zvkb,-zvkg,-zvkn,-zvknc,-zvkned,-zvkng,-zvknha,-zvknhb,-zvks,-zvksc,-zvksed,-zvksg,-zvksh,-zvkt,-zvl1024b,-zvl128b,-zvl16384b,-zvl2048b,-zvl256b,-zvl32768b,-zvl32b,-zvl4096b,-zvl512b,-zvl64b,-zvl65536b,-zvl8192b" }
attributes #2 = { nounwind }
attributes #3 = { nobuiltin "no-builtins" }

!llvm.module.flags = !{!0, !1, !2, !4, !5, !6, !7, !8}
!llvm.ident = !{!9}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 1, !"target-abi", !"lp64d"}
!2 = !{i32 6, !"riscv-isa", !3}
!3 = !{!"rv64i2p1_m2p0_a2p1_f2p2_d2p2_c2p0_zicsr2p0_zifencei2p0_zmmul1p0_zaamo1p0_zalrsc1p0_zca1p0_zcd1p0"}
!4 = !{i32 8, !"PIC Level", i32 2}
!5 = !{i32 7, !"PIE Level", i32 2}
!6 = !{i32 1, !"Code Model", i32 3}
!7 = !{i32 7, !"frame-pointer", i32 2}
!8 = !{i32 8, !"SmallDataLimit", i32 0}
!9 = !{!"clang version 21.1.8 (Fedora 21.1.8-4.fc43)"}
!10 = !{i64 2147535360}
!11 = !{i64 2147535453}
!12 = distinct !{!12, !13}
!13 = !{!"llvm.loop.mustprogress"}
!14 = !{i64 2695}
!15 = distinct !{!15, !13}
!16 = !{i64 1593}
!17 = distinct !{!17, !13}
!18 = distinct !{!18, !13}
