; ModuleID = 'os_isolation_test_v2.ll'
source_filename = "../tca-prototype/os_isolation_test_v2.c"
target datalayout = "e-m:e-p:64:64-i64:64-i128:128-n32:64-S128"
target triple = "riscv64-unknown-unknown-elf"

@dummy_tca_got = dso_local global i64 0, section ".tca_got", align 8
@dummy_tca_sig = dso_local global [64 x i8] zeroinitializer, section ".tca_signatures", align 1
@dummy_cfi = dso_local global i64 0, section ".sentinel_cfi", align 8
@dummy_imports = dso_local global i64 0, section ".sentinel_imports", align 8
@uart = dso_local global ptr inttoptr (i64 268435456 to ptr), align 8
@.str = private unnamed_addr constant [3 x i8] c"0x\00", align 1
@.str.1 = private unnamed_addr constant [2 x i8] c"\0A\00", align 1
@.str.2 = private unnamed_addr constant [57 x i8] c"[M-MODE HYPERVISOR] Intercepted U-mode ecall (Syscall).\0A\00", align 1
@.str.3 = private unnamed_addr constant [38 x i8] c" -> Authenticating intent request...\0A\00", align 1
@.str.4 = private unnamed_addr constant [49 x i8] c" -> Intent Elevated. Writing to CSR_TCA_INTENT.\0A\00", align 1
@.str.5 = private unnamed_addr constant [37 x i8] c"[M-MODE HYPERVISOR] UNHANDLED TRAP!\0A\00", align 1
@.str.6 = private unnamed_addr constant [37 x i8] c"\0A[U-MODE THREAD] Execution started.\0A\00", align 1
@.str.7 = private unnamed_addr constant [62 x i8] c"[U-MODE THREAD] Reading from dynamic Taint Source (ADDR0)...\0A\00", align 1
@.str.8 = private unnamed_addr constant [69 x i8] c"[U-MODE THREAD] Issuing ecall to request 'Network' intent (0x42)...\0A\00", align 1
@.str.9 = private unnamed_addr constant [82 x i8] c"[U-MODE THREAD] Intent granted. Attempting transmission to TX Trigger (ADDR1)...\0A\00", align 1
@.str.10 = private unnamed_addr constant [78 x i8] c"[U-MODE THREAD] Run complete. Execution should have halted via Network Slam.\0A\00", align 1
@.str.11 = private unnamed_addr constant [57 x i8] c"\0A======================================================\0A\00", align 1
@.str.12 = private unnamed_addr constant [50 x i8] c"[TCA V2] OS Isolation & Privilege Ring Benchmark\0A\00", align 1
@.str.13 = private unnamed_addr constant [56 x i8] c"======================================================\0A\00", align 1
@.str.14 = private unnamed_addr constant [59 x i8] c"[M-MODE HYPERVISOR] Configuring Dynamic TCA-PMP bounds...\0A\00", align 1
@.str.15 = private unnamed_addr constant [54 x i8] c"[M-MODE HYPERVISOR] Dropping privileges to U-Mode...\0A\00", align 1

; Function Attrs: noinline nounwind optnone
define dso_local void @llvm_telos_intent_start(i64 noundef %0) #0 {
  %2 = alloca i64, align 8
  store i64 %0, ptr %2, align 8
  ret void
}

; Function Attrs: noinline nounwind optnone
define dso_local void @uart_putc(i8 noundef zeroext %0) #0 {
  %2 = alloca i8, align 1
  store i8 %0, ptr %2, align 1
  %3 = load i8, ptr %2, align 1
  %4 = load ptr, ptr @uart, align 8
  store volatile i8 %3, ptr %4, align 1
  ret void
}

; Function Attrs: noinline nounwind optnone
define dso_local void @uart_puts(ptr noundef %0) #0 {
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
  call void @uart_putc(i8 noundef zeroext %10) #2
  br label %3, !llvm.loop !10

11:                                               ; preds = %3
  ret void
}

; Function Attrs: noinline nounwind optnone
define dso_local void @print_hex(i64 noundef %0) #0 {
  %2 = alloca i64, align 8
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  store i64 %0, ptr %2, align 8
  call void @uart_puts(ptr noundef @.str) #2
  store i32 15, ptr %3, align 4
  br label %5

5:                                                ; preds = %28, %1
  %6 = load i32, ptr %3, align 4
  %7 = icmp sge i32 %6, 0
  br i1 %7, label %8, label %31

8:                                                ; preds = %5
  %9 = load i64, ptr %2, align 8
  %10 = load i32, ptr %3, align 4
  %11 = mul nsw i32 %10, 4
  %12 = zext i32 %11 to i64
  %13 = lshr i64 %9, %12
  %14 = and i64 %13, 15
  %15 = trunc i64 %14 to i32
  store i32 %15, ptr %4, align 4
  %16 = load i32, ptr %4, align 4
  %17 = icmp slt i32 %16, 10
  br i1 %17, label %18, label %22

18:                                               ; preds = %8
  %19 = load i32, ptr %4, align 4
  %20 = add nsw i32 48, %19
  %21 = trunc i32 %20 to i8
  call void @uart_putc(i8 noundef zeroext %21) #2
  br label %27

22:                                               ; preds = %8
  %23 = load i32, ptr %4, align 4
  %24 = sub nsw i32 %23, 10
  %25 = add nsw i32 65, %24
  %26 = trunc i32 %25 to i8
  call void @uart_putc(i8 noundef zeroext %26) #2
  br label %27

27:                                               ; preds = %22, %18
  br label %28

28:                                               ; preds = %27
  %29 = load i32, ptr %3, align 4
  %30 = add nsw i32 %29, -1
  store i32 %30, ptr %3, align 4
  br label %5, !llvm.loop !12

31:                                               ; preds = %5
  call void @uart_puts(ptr noundef @.str.1) #2
  ret void
}

; Function Attrs: noinline nounwind optnone
define dso_local void @trap_handler(i64 noundef %0) #0 align 4 {
  %2 = alloca i64, align 8
  %3 = alloca i64, align 8
  %4 = alloca i64, align 8
  %5 = alloca i64, align 8
  store i64 %0, ptr %2, align 8
  %6 = call i64 asm sideeffect "csrr $0, mcause", "=r"() #3, !srcloc !13
  store i64 %6, ptr %3, align 8
  %7 = call i64 asm sideeffect "csrr $0, mepc", "=r"() #3, !srcloc !14
  store i64 %7, ptr %4, align 8
  %8 = load i64, ptr %3, align 8
  %9 = icmp eq i64 %8, 8
  br i1 %9, label %10, label %16

10:                                               ; preds = %1
  call void @uart_puts(ptr noundef @.str.2) #2
  call void @uart_puts(ptr noundef @.str.3) #2
  %11 = load i64, ptr %2, align 8
  store i64 %11, ptr %5, align 8
  %12 = load i64, ptr %5, align 8
  call void asm sideeffect "csrw 0x800, $0", "r"(i64 %12) #3, !srcloc !15
  call void @uart_puts(ptr noundef @.str.4) #2
  %13 = load i64, ptr %4, align 8
  %14 = add i64 %13, 4
  store i64 %14, ptr %4, align 8
  %15 = load i64, ptr %4, align 8
  call void asm sideeffect "csrw mepc, $0", "r"(i64 %15) #3, !srcloc !16
  call void asm sideeffect "mret", ""() #3, !srcloc !17
  br label %20

16:                                               ; preds = %1
  call void @uart_puts(ptr noundef @.str.5) #2
  %17 = load i64, ptr %3, align 8
  call void @print_hex(i64 noundef %17) #2
  %18 = load i64, ptr %4, align 8
  call void @print_hex(i64 noundef %18) #2
  br label %19

19:                                               ; preds = %19, %16
  br label %19

20:                                               ; preds = %10
  ret void
}

; Function Attrs: naked noinline nounwind optnone
define dso_local void @trap_entry() #1 align 4 {
  call void asm sideeffect "addi sp, sp, -256\0Asd a0, 80(sp)\0Acall trap_handler\0Ald a0, 80(sp)\0Aaddi sp, sp, 256\0Amret\0A", ""() #3, !srcloc !18
  unreachable
}

; Function Attrs: noinline nounwind optnone
define dso_local void @u_mode_thread() #0 {
  %1 = alloca ptr, align 8
  %2 = alloca i64, align 8
  %3 = alloca ptr, align 8
  call void @uart_puts(ptr noundef @.str.6) #2
  store ptr inttoptr (i64 2279604224 to ptr), ptr %1, align 8
  call void @uart_puts(ptr noundef @.str.7) #2
  %4 = load ptr, ptr %1, align 8
  %5 = load volatile i64, ptr %4, align 8
  store i64 %5, ptr %2, align 8
  %6 = load i64, ptr %2, align 8
  call void @uart_puts(ptr noundef @.str.8) #2
  call void asm sideeffect "li a0, 0x42\0Aecall\0A", ""() #3, !srcloc !19
  call void @llvm_telos_intent_start(i64 noundef 66) #2
  store ptr inttoptr (i64 2278555648 to ptr), ptr %3, align 8
  call void @uart_puts(ptr noundef @.str.9) #2
  %7 = load ptr, ptr %3, align 8
  store volatile i64 1, ptr %7, align 8
  call void @uart_puts(ptr noundef @.str.10) #2
  br label %8

8:                                                ; preds = %8, %0
  br label %8
}

; Function Attrs: noinline nounwind optnone
define dso_local void @_start() #0 {
  %1 = alloca i64, align 8
  %2 = alloca i64, align 8
  %3 = alloca i64, align 8
  %4 = alloca i64, align 8
  %5 = alloca i64, align 8
  %6 = alloca i64, align 8
  %7 = alloca i64, align 8
  %8 = alloca i64, align 8
  call void @uart_puts(ptr noundef @.str.11) #2
  call void @uart_puts(ptr noundef @.str.12) #2
  call void @uart_puts(ptr noundef @.str.13) #2
  %9 = and i64 ptrtoint (ptr @trap_entry to i64), -4
  call void asm sideeffect "csrw mtvec, $0", "r"(i64 %9) #3, !srcloc !20
  store i64 -1, ptr %1, align 8
  store i64 15, ptr %2, align 8
  %10 = load i64, ptr %1, align 8
  call void asm sideeffect "csrw pmpaddr0, $0", "r"(i64 %10) #3, !srcloc !21
  %11 = load i64, ptr %2, align 8
  call void asm sideeffect "csrw pmpcfg0, $0", "r"(i64 %11) #3, !srcloc !22
  call void @uart_puts(ptr noundef @.str.14) #2
  store i64 2279604224, ptr %3, align 8
  store i64 2278555648, ptr %4, align 8
  store i64 66, ptr %5, align 8
  store i64 10, ptr %6, align 8
  %12 = load i64, ptr %5, align 8
  %13 = shl i64 %12, 32
  %14 = load i64, ptr %6, align 8
  %15 = or i64 %13, %14
  store i64 %15, ptr %7, align 8
  %16 = load i64, ptr %3, align 8
  call void asm sideeffect "csrw 0x803, $0", "r"(i64 %16) #3, !srcloc !23
  %17 = load i64, ptr %4, align 8
  call void asm sideeffect "csrw 0x804, $0", "r"(i64 %17) #3, !srcloc !24
  %18 = load i64, ptr %7, align 8
  call void asm sideeffect "csrw 0x802, $0", "r"(i64 %18) #3, !srcloc !25
  call void @uart_puts(ptr noundef @.str.15) #2
  %19 = call i64 asm sideeffect "csrr $0, mstatus", "=r"() #3, !srcloc !26
  store i64 %19, ptr %8, align 8
  %20 = load i64, ptr %8, align 8
  %21 = and i64 %20, -6145
  store i64 %21, ptr %8, align 8
  %22 = load i64, ptr %8, align 8
  call void asm sideeffect "csrw mstatus, $0", "r"(i64 %22) #3, !srcloc !27
  call void asm sideeffect "csrw mepc, $0", "r"(i64 ptrtoint (ptr @u_mode_thread to i64)) #3, !srcloc !28
  call void asm sideeffect "mret", ""() #3, !srcloc !29
  br label %23

23:                                               ; preds = %23, %0
  br label %23
}

attributes #0 = { noinline nounwind optnone "frame-pointer"="all" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="generic-rv64" "target-features"="+64bit,+a,+c,+d,+f,+m,+relax,+zaamo,+zalrsc,+zca,+zcd,+zicsr,+zifencei,+zmmul,-b,-e,-experimental-p,-experimental-smctr,-experimental-ssctr,-experimental-svukte,-experimental-xqccmp,-experimental-xqcia,-experimental-xqciac,-experimental-xqcibi,-experimental-xqcibm,-experimental-xqcicli,-experimental-xqcicm,-experimental-xqcics,-experimental-xqcicsr,-experimental-xqciint,-experimental-xqciio,-experimental-xqcilb,-experimental-xqcili,-experimental-xqcilia,-experimental-xqcilo,-experimental-xqcilsm,-experimental-xqcisim,-experimental-xqcisls,-experimental-xqcisync,-experimental-xrivosvisni,-experimental-xrivosvizip,-experimental-xsfmclic,-experimental-xsfsclic,-experimental-zalasr,-experimental-zicfilp,-experimental-zicfiss,-experimental-zvbc32e,-experimental-zvkgs,-experimental-zvqdotq,-h,-q,-sdext,-sdtrig,-sha,-shcounterenw,-shgatpa,-shlcofideleg,-shtvala,-shvsatpa,-shvstvala,-shvstvecd,-smaia,-smcdeleg,-smcntrpmf,-smcsrind,-smdbltrp,-smepmp,-smmpm,-smnpm,-smrnmi,-smstateen,-ssaia,-ssccfg,-ssccptr,-sscofpmf,-sscounterenw,-sscsrind,-ssdbltrp,-ssnpm,-sspm,-ssqosid,-ssstateen,-ssstrict,-sstc,-sstvala,-sstvecd,-ssu64xl,-supm,-svade,-svadu,-svbare,-svinval,-svnapot,-svpbmt,-svvptc,-v,-xandesbfhcvt,-xandesperf,-xandesvbfhcvt,-xandesvdot,-xandesvpackfph,-xandesvsintload,-xcvalu,-xcvbi,-xcvbitmanip,-xcvelw,-xcvmac,-xcvmem,-xcvsimd,-xmipscbop,-xmipscmov,-xmipslsp,-xsfcease,-xsfmm128t,-xsfmm16t,-xsfmm32a16f,-xsfmm32a32f,-xsfmm32a8f,-xsfmm32a8i,-xsfmm32t,-xsfmm64a64f,-xsfmm64t,-xsfmmbase,-xsfvcp,-xsfvfnrclipxfqf,-xsfvfwmaccqqq,-xsfvqmaccdod,-xsfvqmaccqoq,-xsifivecdiscarddlone,-xsifivecflushdlone,-xtheadba,-xtheadbb,-xtheadbs,-xtheadcmo,-xtheadcondmov,-xtheadfmemidx,-xtheadmac,-xtheadmemidx,-xtheadmempair,-xtheadsync,-xtheadvdot,-xventanacondops,-xwchc,-za128rs,-za64rs,-zabha,-zacas,-zama16b,-zawrs,-zba,-zbb,-zbc,-zbkb,-zbkc,-zbkx,-zbs,-zcb,-zce,-zcf,-zclsd,-zcmop,-zcmp,-zcmt,-zdinx,-zfa,-zfbfmin,-zfh,-zfhmin,-zfinx,-zhinx,-zhinxmin,-zic64b,-zicbom,-zicbop,-zicboz,-ziccamoa,-ziccamoc,-ziccif,-zicclsm,-ziccrse,-zicntr,-zicond,-zihintntl,-zihintpause,-zihpm,-zilsd,-zimop,-zk,-zkn,-zknd,-zkne,-zknh,-zkr,-zks,-zksed,-zksh,-zkt,-ztso,-zvbb,-zvbc,-zve32f,-zve32x,-zve64d,-zve64f,-zve64x,-zvfbfmin,-zvfbfwma,-zvfh,-zvfhmin,-zvkb,-zvkg,-zvkn,-zvknc,-zvkned,-zvkng,-zvknha,-zvknhb,-zvks,-zvksc,-zvksed,-zvksg,-zvksh,-zvkt,-zvl1024b,-zvl128b,-zvl16384b,-zvl2048b,-zvl256b,-zvl32768b,-zvl32b,-zvl4096b,-zvl512b,-zvl64b,-zvl65536b,-zvl8192b" }
attributes #1 = { naked noinline nounwind optnone "frame-pointer"="all" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="generic-rv64" "target-features"="+64bit,+a,+c,+d,+f,+m,+relax,+zaamo,+zalrsc,+zca,+zcd,+zicsr,+zifencei,+zmmul,-b,-e,-experimental-p,-experimental-smctr,-experimental-ssctr,-experimental-svukte,-experimental-xqccmp,-experimental-xqcia,-experimental-xqciac,-experimental-xqcibi,-experimental-xqcibm,-experimental-xqcicli,-experimental-xqcicm,-experimental-xqcics,-experimental-xqcicsr,-experimental-xqciint,-experimental-xqciio,-experimental-xqcilb,-experimental-xqcili,-experimental-xqcilia,-experimental-xqcilo,-experimental-xqcilsm,-experimental-xqcisim,-experimental-xqcisls,-experimental-xqcisync,-experimental-xrivosvisni,-experimental-xrivosvizip,-experimental-xsfmclic,-experimental-xsfsclic,-experimental-zalasr,-experimental-zicfilp,-experimental-zicfiss,-experimental-zvbc32e,-experimental-zvkgs,-experimental-zvqdotq,-h,-q,-sdext,-sdtrig,-sha,-shcounterenw,-shgatpa,-shlcofideleg,-shtvala,-shvsatpa,-shvstvala,-shvstvecd,-smaia,-smcdeleg,-smcntrpmf,-smcsrind,-smdbltrp,-smepmp,-smmpm,-smnpm,-smrnmi,-smstateen,-ssaia,-ssccfg,-ssccptr,-sscofpmf,-sscounterenw,-sscsrind,-ssdbltrp,-ssnpm,-sspm,-ssqosid,-ssstateen,-ssstrict,-sstc,-sstvala,-sstvecd,-ssu64xl,-supm,-svade,-svadu,-svbare,-svinval,-svnapot,-svpbmt,-svvptc,-v,-xandesbfhcvt,-xandesperf,-xandesvbfhcvt,-xandesvdot,-xandesvpackfph,-xandesvsintload,-xcvalu,-xcvbi,-xcvbitmanip,-xcvelw,-xcvmac,-xcvmem,-xcvsimd,-xmipscbop,-xmipscmov,-xmipslsp,-xsfcease,-xsfmm128t,-xsfmm16t,-xsfmm32a16f,-xsfmm32a32f,-xsfmm32a8f,-xsfmm32a8i,-xsfmm32t,-xsfmm64a64f,-xsfmm64t,-xsfmmbase,-xsfvcp,-xsfvfnrclipxfqf,-xsfvfwmaccqqq,-xsfvqmaccdod,-xsfvqmaccqoq,-xsifivecdiscarddlone,-xsifivecflushdlone,-xtheadba,-xtheadbb,-xtheadbs,-xtheadcmo,-xtheadcondmov,-xtheadfmemidx,-xtheadmac,-xtheadmemidx,-xtheadmempair,-xtheadsync,-xtheadvdot,-xventanacondops,-xwchc,-za128rs,-za64rs,-zabha,-zacas,-zama16b,-zawrs,-zba,-zbb,-zbc,-zbkb,-zbkc,-zbkx,-zbs,-zcb,-zce,-zcf,-zclsd,-zcmop,-zcmp,-zcmt,-zdinx,-zfa,-zfbfmin,-zfh,-zfhmin,-zfinx,-zhinx,-zhinxmin,-zic64b,-zicbom,-zicbop,-zicboz,-ziccamoa,-ziccamoc,-ziccif,-zicclsm,-ziccrse,-zicntr,-zicond,-zihintntl,-zihintpause,-zihpm,-zilsd,-zimop,-zk,-zkn,-zknd,-zkne,-zknh,-zkr,-zks,-zksed,-zksh,-zkt,-ztso,-zvbb,-zvbc,-zve32f,-zve32x,-zve64d,-zve64f,-zve64x,-zvfbfmin,-zvfbfwma,-zvfh,-zvfhmin,-zvkb,-zvkg,-zvkn,-zvknc,-zvkned,-zvkng,-zvknha,-zvknhb,-zvks,-zvksc,-zvksed,-zvksg,-zvksh,-zvkt,-zvl1024b,-zvl128b,-zvl16384b,-zvl2048b,-zvl256b,-zvl32768b,-zvl32b,-zvl4096b,-zvl512b,-zvl64b,-zvl65536b,-zvl8192b" }
attributes #2 = { nobuiltin "no-builtins" }
attributes #3 = { nounwind }

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
!10 = distinct !{!10, !11}
!11 = !{!"llvm.loop.mustprogress"}
!12 = distinct !{!12, !11}
!13 = !{i64 1222}
!14 = !{i64 1278}
!15 = !{i64 1785}
!16 = !{i64 2025}
!17 = !{i64 2119}
!18 = !{i64 2366, i64 2397, i64 2423, i64 2453, i64 2479, i64 2508}
!19 = !{i64 3295, i64 3312}
!20 = !{i64 4148}
!21 = !{i64 4401}
!22 = !{i64 4462}
!23 = !{i64 4868}
!24 = !{i64 4944}
!25 = !{i64 5020}
!26 = !{i64 5222}
!27 = !{i64 5343}
!28 = !{i64 5402}
!29 = !{i64 5510}
