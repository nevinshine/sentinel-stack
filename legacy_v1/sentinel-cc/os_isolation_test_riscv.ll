; ModuleID = '../tca-prototype/os_isolation_test.c'
source_filename = "../tca-prototype/os_isolation_test.c"
target datalayout = "e-m:e-p:64:64-i64:64-i128:128-n32:64-S128"
target triple = "riscv64-unknown-unknown-elf"

%struct.ThreadContext = type { i64 }

@__sentinel_signature = dso_local global [64 x i8] zeroinitializer, section ".signature", align 1
@.str = private unnamed_addr constant [60 x i8] c"[THREAD B] Woke up. Executing in expected clean context...\0A\00", align 1
@.str.1 = private unnamed_addr constant [25 x i8] c"network_send_socket_data\00", align 1
@.str.2 = private unnamed_addr constant [52 x i8] c"[THREAD B] Attempting to transmit on TX Trigger...\0A\00", align 1
@.str.3 = private unnamed_addr constant [58 x i8] c"[THREAD B] TX Write finished. Yielding back to Thread A.\0A\00", align 1
@thread_b = dso_local global %struct.ThreadContext zeroinitializer, align 8
@thread_a = dso_local global %struct.ThreadContext zeroinitializer, align 8
@.str.4 = private unnamed_addr constant [42 x i8] c"========================================\0A\00", align 1
@.str.5 = private unnamed_addr constant [25 x i8] c"[TCA] OS Isolation Test\0A\00", align 1
@stack_b = dso_local global [4096 x i8] zeroinitializer, align 16
@.str.6 = private unnamed_addr constant [55 x i8] c"[THREAD A] Reading from sensitive (tainted) memory...\0A\00", align 1
@.str.7 = private unnamed_addr constant [36 x i8] c"[THREAD A] Yielding to Thread B...\0A\00", align 1
@.str.8 = private unnamed_addr constant [44 x i8] c"[THREAD A] Woke back up. Context restored.\0A\00", align 1
@.str.9 = private unnamed_addr constant [52 x i8] c"[THREAD A] Attempting to transmit on TX Trigger...\0A\00", align 1
@.str.10 = private unnamed_addr constant [80 x i8] c"[THREAD A] Run complete (Should not be reached if NIC drops the transmission).\0A\00", align 1

; Function Attrs: noinline nounwind optnone
define dso_local void @tca_set_intent(ptr noundef %0, i64 noundef %1) #0 {
  %3 = alloca ptr, align 8
  %4 = alloca i64, align 8
  store ptr %0, ptr %3, align 8
  store i64 %1, ptr %4, align 8
  %5 = load ptr, ptr %3, align 8
  %6 = load i64, ptr %4, align 8
  call void asm sideeffect ".insn r 0x0B, 0x0, 0x01, x0, $0, $1", "r,r"(ptr %5, i64 %6) #3, !srcloc !10
  ret void
}

; Function Attrs: noinline nounwind optnone
define dso_local void @tca_clear() #0 {
  call void asm sideeffect ".insn r 0x0B, 0x0, 0x02, x0, x0, x0", ""() #3, !srcloc !11
  ret void
}

; Function Attrs: naked noinline nounwind optnone
define dso_local void @switch_context(ptr noundef %0, ptr noundef %1) #1 {
  call void asm sideeffect "addi sp, sp, -128\0Asd ra,   0(sp)\0Asd s0,   8(sp)\0Asd s1,  16(sp)\0Asd s2,  24(sp)\0Asd s3,  32(sp)\0Asd s4,  40(sp)\0Asd s5,  48(sp)\0Asd s6,  56(sp)\0Asd s7,  64(sp)\0Asd s8,  72(sp)\0Asd s9,  80(sp)\0Asd s10, 88(sp)\0Asd s11, 96(sp)\0Acsrr t0, 0x800\0Asd   t0, 104(sp)\0Acsrr t1, 0x801\0Asd   t1, 112(sp)\0Asd sp, 0(a0)\0Ald sp, 0(a1)\0Ald   t0, 104(sp)\0Acsrw 0x800, t0\0Ald   t1, 112(sp)\0Acsrw 0x801, t1\0Ald ra,   0(sp)\0Ald s0,   8(sp)\0Ald s1,  16(sp)\0Ald s2,  24(sp)\0Ald s3,  32(sp)\0Ald s4,  40(sp)\0Ald s5,  48(sp)\0Ald s6,  56(sp)\0Ald s7,  64(sp)\0Ald s8,  72(sp)\0Ald s9,  80(sp)\0Ald s10, 88(sp)\0Ald s11, 96(sp)\0Aaddi sp, sp, 128\0Aret\0A", ""() #3, !srcloc !12
  unreachable
}

; Function Attrs: noinline nounwind optnone
define dso_local void @thread_b_func() #0 {
  %1 = alloca ptr, align 8
  call void @uart_puts(ptr noundef @.str) #4
  call void @llvm.telos.intent.start(ptr noundef @.str.1) #4
  store ptr inttoptr (i64 2278555648 to ptr), ptr %1, align 8
  call void @uart_puts(ptr noundef @.str.2) #4
  %2 = load ptr, ptr %1, align 8
  store volatile i64 1, ptr %2, align 8
  call void @uart_puts(ptr noundef @.str.3) #4
  call void @switch_context(ptr noundef @thread_b, ptr noundef @thread_a) #4
  br label %3

3:                                                ; preds = %0, %3
  call void asm sideeffect "wfi", ""() #3, !srcloc !13
  br label %3
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
  call void @uart_putc(i8 noundef zeroext %10) #4
  br label %3, !llvm.loop !14

11:                                               ; preds = %3
  ret void
}

; Unknown intrinsic
declare void @llvm.telos.intent.start(ptr noundef) #2

; Function Attrs: noinline nounwind optnone
define dso_local void @_start() #0 {
  %1 = alloca ptr, align 8
  %2 = alloca ptr, align 8
  %3 = alloca ptr, align 8
  %4 = alloca ptr, align 8
  %5 = alloca i64, align 8
  call void @uart_puts(ptr noundef @.str.4) #4
  call void @uart_puts(ptr noundef @.str.5) #4
  call void @uart_puts(ptr noundef @.str.4) #4
  store ptr @__sentinel_signature, ptr %1, align 8
  %6 = load ptr, ptr %1, align 8
  call void @llvm.telos.intent.start(ptr noundef @.str.1) #4
  store ptr getelementptr inbounds nuw (i8, ptr @stack_b, i64 4096), ptr %2, align 8
  %7 = load ptr, ptr %2, align 8
  %8 = getelementptr inbounds i64, ptr %7, i64 -16
  store ptr %8, ptr %2, align 8
  %9 = load ptr, ptr %2, align 8
  %10 = getelementptr inbounds i64, ptr %9, i64 0
  store i64 ptrtoint (ptr @thread_b_func to i64), ptr %10, align 8
  %11 = load ptr, ptr %2, align 8
  %12 = getelementptr inbounds i64, ptr %11, i64 13
  store i64 0, ptr %12, align 8
  %13 = load ptr, ptr %2, align 8
  %14 = getelementptr inbounds i64, ptr %13, i64 14
  store i64 0, ptr %14, align 8
  %15 = load ptr, ptr %2, align 8
  %16 = ptrtoint ptr %15 to i64
  store i64 %16, ptr @thread_b, align 8
  store ptr inttoptr (i64 2279604224 to ptr), ptr %3, align 8
  store ptr inttoptr (i64 2278555648 to ptr), ptr %4, align 8
  call void @uart_puts(ptr noundef @.str.6) #4
  %17 = load ptr, ptr %3, align 8
  %18 = load volatile i64, ptr %17, align 8
  store i64 %18, ptr %5, align 8
  %19 = load i64, ptr %5, align 8
  call void @uart_puts(ptr noundef @.str.7) #4
  call void @switch_context(ptr noundef @thread_a, ptr noundef @thread_b) #4
  call void @uart_puts(ptr noundef @.str.8) #4
  call void @uart_puts(ptr noundef @.str.9) #4
  %20 = load ptr, ptr %4, align 8
  store volatile i64 1, ptr %20, align 8
  call void @uart_puts(ptr noundef @.str.10) #4
  call void @llvm.telos.intent.end() #4
  br label %21

21:                                               ; preds = %0, %21
  call void asm sideeffect "wfi", ""() #3, !srcloc !16
  br label %21
}

; Unknown intrinsic
declare void @llvm.telos.intent.end() #2

; Function Attrs: noinline nounwind optnone
define internal void @uart_putc(i8 noundef zeroext %0) #0 {
  %2 = alloca i8, align 1
  %3 = alloca ptr, align 8
  store i8 %0, ptr %2, align 1
  store ptr inttoptr (i64 268435456 to ptr), ptr %3, align 8
  %4 = load i8, ptr %2, align 1
  %5 = load ptr, ptr %3, align 8
  %6 = getelementptr inbounds i8, ptr %5, i64 0
  store volatile i8 %4, ptr %6, align 1
  ret void
}

attributes #0 = { noinline nounwind optnone "frame-pointer"="all" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="generic-rv64" "target-features"="+64bit,+a,+c,+d,+f,+m,+relax,+zaamo,+zalrsc,+zca,+zcd,+zicsr,+zifencei,+zmmul,-b,-e,-experimental-p,-experimental-smctr,-experimental-ssctr,-experimental-svukte,-experimental-xqccmp,-experimental-xqcia,-experimental-xqciac,-experimental-xqcibi,-experimental-xqcibm,-experimental-xqcicli,-experimental-xqcicm,-experimental-xqcics,-experimental-xqcicsr,-experimental-xqciint,-experimental-xqciio,-experimental-xqcilb,-experimental-xqcili,-experimental-xqcilia,-experimental-xqcilo,-experimental-xqcilsm,-experimental-xqcisim,-experimental-xqcisls,-experimental-xqcisync,-experimental-xrivosvisni,-experimental-xrivosvizip,-experimental-xsfmclic,-experimental-xsfsclic,-experimental-zalasr,-experimental-zicfilp,-experimental-zicfiss,-experimental-zvbc32e,-experimental-zvkgs,-experimental-zvqdotq,-h,-q,-sdext,-sdtrig,-sha,-shcounterenw,-shgatpa,-shlcofideleg,-shtvala,-shvsatpa,-shvstvala,-shvstvecd,-smaia,-smcdeleg,-smcntrpmf,-smcsrind,-smdbltrp,-smepmp,-smmpm,-smnpm,-smrnmi,-smstateen,-ssaia,-ssccfg,-ssccptr,-sscofpmf,-sscounterenw,-sscsrind,-ssdbltrp,-ssnpm,-sspm,-ssqosid,-ssstateen,-ssstrict,-sstc,-sstvala,-sstvecd,-ssu64xl,-supm,-svade,-svadu,-svbare,-svinval,-svnapot,-svpbmt,-svvptc,-v,-xandesbfhcvt,-xandesperf,-xandesvbfhcvt,-xandesvdot,-xandesvpackfph,-xandesvsintload,-xcvalu,-xcvbi,-xcvbitmanip,-xcvelw,-xcvmac,-xcvmem,-xcvsimd,-xmipscbop,-xmipscmov,-xmipslsp,-xsfcease,-xsfmm128t,-xsfmm16t,-xsfmm32a16f,-xsfmm32a32f,-xsfmm32a8f,-xsfmm32a8i,-xsfmm32t,-xsfmm64a64f,-xsfmm64t,-xsfmmbase,-xsfvcp,-xsfvfnrclipxfqf,-xsfvfwmaccqqq,-xsfvqmaccdod,-xsfvqmaccqoq,-xsifivecdiscarddlone,-xsifivecflushdlone,-xtheadba,-xtheadbb,-xtheadbs,-xtheadcmo,-xtheadcondmov,-xtheadfmemidx,-xtheadmac,-xtheadmemidx,-xtheadmempair,-xtheadsync,-xtheadvdot,-xventanacondops,-xwchc,-za128rs,-za64rs,-zabha,-zacas,-zama16b,-zawrs,-zba,-zbb,-zbc,-zbkb,-zbkc,-zbkx,-zbs,-zcb,-zce,-zcf,-zclsd,-zcmop,-zcmp,-zcmt,-zdinx,-zfa,-zfbfmin,-zfh,-zfhmin,-zfinx,-zhinx,-zhinxmin,-zic64b,-zicbom,-zicbop,-zicboz,-ziccamoa,-ziccamoc,-ziccif,-zicclsm,-ziccrse,-zicntr,-zicond,-zihintntl,-zihintpause,-zihpm,-zilsd,-zimop,-zk,-zkn,-zknd,-zkne,-zknh,-zkr,-zks,-zksed,-zksh,-zkt,-ztso,-zvbb,-zvbc,-zve32f,-zve32x,-zve64d,-zve64f,-zve64x,-zvfbfmin,-zvfbfwma,-zvfh,-zvfhmin,-zvkb,-zvkg,-zvkn,-zvknc,-zvkned,-zvkng,-zvknha,-zvknhb,-zvks,-zvksc,-zvksed,-zvksg,-zvksh,-zvkt,-zvl1024b,-zvl128b,-zvl16384b,-zvl2048b,-zvl256b,-zvl32768b,-zvl32b,-zvl4096b,-zvl512b,-zvl64b,-zvl65536b,-zvl8192b" }
attributes #1 = { naked noinline nounwind optnone "frame-pointer"="all" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="generic-rv64" "target-features"="+64bit,+a,+c,+d,+f,+m,+relax,+zaamo,+zalrsc,+zca,+zcd,+zicsr,+zifencei,+zmmul,-b,-e,-experimental-p,-experimental-smctr,-experimental-ssctr,-experimental-svukte,-experimental-xqccmp,-experimental-xqcia,-experimental-xqciac,-experimental-xqcibi,-experimental-xqcibm,-experimental-xqcicli,-experimental-xqcicm,-experimental-xqcics,-experimental-xqcicsr,-experimental-xqciint,-experimental-xqciio,-experimental-xqcilb,-experimental-xqcili,-experimental-xqcilia,-experimental-xqcilo,-experimental-xqcilsm,-experimental-xqcisim,-experimental-xqcisls,-experimental-xqcisync,-experimental-xrivosvisni,-experimental-xrivosvizip,-experimental-xsfmclic,-experimental-xsfsclic,-experimental-zalasr,-experimental-zicfilp,-experimental-zicfiss,-experimental-zvbc32e,-experimental-zvkgs,-experimental-zvqdotq,-h,-q,-sdext,-sdtrig,-sha,-shcounterenw,-shgatpa,-shlcofideleg,-shtvala,-shvsatpa,-shvstvala,-shvstvecd,-smaia,-smcdeleg,-smcntrpmf,-smcsrind,-smdbltrp,-smepmp,-smmpm,-smnpm,-smrnmi,-smstateen,-ssaia,-ssccfg,-ssccptr,-sscofpmf,-sscounterenw,-sscsrind,-ssdbltrp,-ssnpm,-sspm,-ssqosid,-ssstateen,-ssstrict,-sstc,-sstvala,-sstvecd,-ssu64xl,-supm,-svade,-svadu,-svbare,-svinval,-svnapot,-svpbmt,-svvptc,-v,-xandesbfhcvt,-xandesperf,-xandesvbfhcvt,-xandesvdot,-xandesvpackfph,-xandesvsintload,-xcvalu,-xcvbi,-xcvbitmanip,-xcvelw,-xcvmac,-xcvmem,-xcvsimd,-xmipscbop,-xmipscmov,-xmipslsp,-xsfcease,-xsfmm128t,-xsfmm16t,-xsfmm32a16f,-xsfmm32a32f,-xsfmm32a8f,-xsfmm32a8i,-xsfmm32t,-xsfmm64a64f,-xsfmm64t,-xsfmmbase,-xsfvcp,-xsfvfnrclipxfqf,-xsfvfwmaccqqq,-xsfvqmaccdod,-xsfvqmaccqoq,-xsifivecdiscarddlone,-xsifivecflushdlone,-xtheadba,-xtheadbb,-xtheadbs,-xtheadcmo,-xtheadcondmov,-xtheadfmemidx,-xtheadmac,-xtheadmemidx,-xtheadmempair,-xtheadsync,-xtheadvdot,-xventanacondops,-xwchc,-za128rs,-za64rs,-zabha,-zacas,-zama16b,-zawrs,-zba,-zbb,-zbc,-zbkb,-zbkc,-zbkx,-zbs,-zcb,-zce,-zcf,-zclsd,-zcmop,-zcmp,-zcmt,-zdinx,-zfa,-zfbfmin,-zfh,-zfhmin,-zfinx,-zhinx,-zhinxmin,-zic64b,-zicbom,-zicbop,-zicboz,-ziccamoa,-ziccamoc,-ziccif,-zicclsm,-ziccrse,-zicntr,-zicond,-zihintntl,-zihintpause,-zihpm,-zilsd,-zimop,-zk,-zkn,-zknd,-zkne,-zknh,-zkr,-zks,-zksed,-zksh,-zkt,-ztso,-zvbb,-zvbc,-zve32f,-zve32x,-zve64d,-zve64f,-zve64x,-zvfbfmin,-zvfbfwma,-zvfh,-zvfhmin,-zvkb,-zvkg,-zvkn,-zvknc,-zvkned,-zvkng,-zvknha,-zvknhb,-zvks,-zvksc,-zvksed,-zvksg,-zvksh,-zvkt,-zvl1024b,-zvl128b,-zvl16384b,-zvl2048b,-zvl256b,-zvl32768b,-zvl32b,-zvl4096b,-zvl512b,-zvl64b,-zvl65536b,-zvl8192b" }
attributes #2 = { "frame-pointer"="all" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="generic-rv64" "target-features"="+64bit,+a,+c,+d,+f,+m,+relax,+zaamo,+zalrsc,+zca,+zcd,+zicsr,+zifencei,+zmmul,-b,-e,-experimental-p,-experimental-smctr,-experimental-ssctr,-experimental-svukte,-experimental-xqccmp,-experimental-xqcia,-experimental-xqciac,-experimental-xqcibi,-experimental-xqcibm,-experimental-xqcicli,-experimental-xqcicm,-experimental-xqcics,-experimental-xqcicsr,-experimental-xqciint,-experimental-xqciio,-experimental-xqcilb,-experimental-xqcili,-experimental-xqcilia,-experimental-xqcilo,-experimental-xqcilsm,-experimental-xqcisim,-experimental-xqcisls,-experimental-xqcisync,-experimental-xrivosvisni,-experimental-xrivosvizip,-experimental-xsfmclic,-experimental-xsfsclic,-experimental-zalasr,-experimental-zicfilp,-experimental-zicfiss,-experimental-zvbc32e,-experimental-zvkgs,-experimental-zvqdotq,-h,-q,-sdext,-sdtrig,-sha,-shcounterenw,-shgatpa,-shlcofideleg,-shtvala,-shvsatpa,-shvstvala,-shvstvecd,-smaia,-smcdeleg,-smcntrpmf,-smcsrind,-smdbltrp,-smepmp,-smmpm,-smnpm,-smrnmi,-smstateen,-ssaia,-ssccfg,-ssccptr,-sscofpmf,-sscounterenw,-sscsrind,-ssdbltrp,-ssnpm,-sspm,-ssqosid,-ssstateen,-ssstrict,-sstc,-sstvala,-sstvecd,-ssu64xl,-supm,-svade,-svadu,-svbare,-svinval,-svnapot,-svpbmt,-svvptc,-v,-xandesbfhcvt,-xandesperf,-xandesvbfhcvt,-xandesvdot,-xandesvpackfph,-xandesvsintload,-xcvalu,-xcvbi,-xcvbitmanip,-xcvelw,-xcvmac,-xcvmem,-xcvsimd,-xmipscbop,-xmipscmov,-xmipslsp,-xsfcease,-xsfmm128t,-xsfmm16t,-xsfmm32a16f,-xsfmm32a32f,-xsfmm32a8f,-xsfmm32a8i,-xsfmm32t,-xsfmm64a64f,-xsfmm64t,-xsfmmbase,-xsfvcp,-xsfvfnrclipxfqf,-xsfvfwmaccqqq,-xsfvqmaccdod,-xsfvqmaccqoq,-xsifivecdiscarddlone,-xsifivecflushdlone,-xtheadba,-xtheadbb,-xtheadbs,-xtheadcmo,-xtheadcondmov,-xtheadfmemidx,-xtheadmac,-xtheadmemidx,-xtheadmempair,-xtheadsync,-xtheadvdot,-xventanacondops,-xwchc,-za128rs,-za64rs,-zabha,-zacas,-zama16b,-zawrs,-zba,-zbb,-zbc,-zbkb,-zbkc,-zbkx,-zbs,-zcb,-zce,-zcf,-zclsd,-zcmop,-zcmp,-zcmt,-zdinx,-zfa,-zfbfmin,-zfh,-zfhmin,-zfinx,-zhinx,-zhinxmin,-zic64b,-zicbom,-zicbop,-zicboz,-ziccamoa,-ziccamoc,-ziccif,-zicclsm,-ziccrse,-zicntr,-zicond,-zihintntl,-zihintpause,-zihpm,-zilsd,-zimop,-zk,-zkn,-zknd,-zkne,-zknh,-zkr,-zks,-zksed,-zksh,-zkt,-ztso,-zvbb,-zvbc,-zve32f,-zve32x,-zve64d,-zve64f,-zve64x,-zvfbfmin,-zvfbfwma,-zvfh,-zvfhmin,-zvkb,-zvkg,-zvkn,-zvknc,-zvkned,-zvkng,-zvknha,-zvknhb,-zvks,-zvksc,-zvksed,-zvksg,-zvksh,-zvkt,-zvl1024b,-zvl128b,-zvl16384b,-zvl2048b,-zvl256b,-zvl32768b,-zvl32b,-zvl4096b,-zvl512b,-zvl64b,-zvl65536b,-zvl8192b" }
attributes #3 = { nounwind }
attributes #4 = { nobuiltin "no-builtins" }

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
!10 = !{i64 2147535468}
!11 = !{i64 2147535561}
!12 = !{i64 1451, i64 1556, i64 1583, i64 1610, i64 1637, i64 1664, i64 1691, i64 1718, i64 1745, i64 1772, i64 1799, i64 1826, i64 1853, i64 1880, i64 1967, i64 2027, i64 2062, i64 2121, i64 2192, i64 2246, i64 2363, i64 2392, i64 2453, i64 2482, i64 2600, i64 2627, i64 2654, i64 2681, i64 2708, i64 2735, i64 2762, i64 2789, i64 2816, i64 2843, i64 2870, i64 2897, i64 2924, i64 3003, i64 3032}
!13 = !{i64 3804}
!14 = distinct !{!14, !15}
!15 = !{!"llvm.loop.mustprogress"}
!16 = !{i64 5601}
