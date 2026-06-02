; ModuleID = 'exfil_test_riscv.ll'
source_filename = "../tca-prototype/exfil_test.c"
target datalayout = "e-m:e-p:64:64-i64:64-i128:128-n32:64-S128"
target triple = "riscv64-unknown-unknown-elf"

@__sentinel_signature = dso_local global [64 x i8] zeroinitializer, section ".signature", align 1
@.str = private unnamed_addr constant [42 x i8] c"========================================\0A\00", align 1
@.str.1 = private unnamed_addr constant [38 x i8] c"[TCA] Quantum-State IFC Network Slam\0A\00", align 1
@.str.2 = private unnamed_addr constant [49 x i8] c"[DIAG] Phase 1: Setting valid network intent...\0A\00", align 1
@.str.3 = private unnamed_addr constant [25 x i8] c"network_send_socket_data\00", align 1
@.str.4 = private unnamed_addr constant [64 x i8] c"[DIAG] Valid intent activated. We are authorized to send data.\0A\00", align 1
@.str.5 = private unnamed_addr constant [60 x i8] c"[DIAG] Phase 2: Reading from sensitive (tainted) memory...\0A\00", align 1
@.str.6 = private unnamed_addr constant [60 x i8] c"[DIAG] Memory read. Taint should be physically propagated.\0A\00", align 1
@.str.7 = private unnamed_addr constant [66 x i8] c"[DIAG] Phase 3: Attempting to flush data to Network Interface...\0A\00", align 1
@.str.8 = private unnamed_addr constant [41 x i8] c"[DIAG] Exfiltration benchmark complete.\0A\00", align 1
@.tca_got_table = constant [1 x i64] [i64 -8526495043115022390], section ".tca_got", align 8

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
define dso_local void @_start() #0 {
  %1 = alloca ptr, align 8
  %2 = alloca ptr, align 8
  %3 = alloca ptr, align 8
  %4 = alloca i64, align 8
  call void @uart_init() #3
  store ptr @__sentinel_signature, ptr %1, align 8
  %5 = load ptr, ptr %1, align 8
  call void @uart_puts(ptr noundef @.str) #3
  call void @uart_puts(ptr noundef @.str.1) #3
  call void @uart_puts(ptr noundef @.str) #3
  store ptr inttoptr (i64 2279604224 to ptr), ptr %2, align 8
  store ptr inttoptr (i64 2278555648 to ptr), ptr %3, align 8
  call void @uart_puts(ptr noundef @.str.2) #3
  %tca_got_load = load volatile i64, ptr @.tca_got_table, align 8
  %6 = call ptr addrspace(200) @tca_set_intent(ptr addrspace(200) addrspacecast (ptr @_start to ptr addrspace(200)), i64 %tca_got_load)
  call void @uart_puts(ptr noundef @.str.4) #3
  call void @uart_puts(ptr noundef @.str.5) #3
  %7 = load ptr, ptr %2, align 8
  %8 = load volatile i64, ptr %7, align 8
  store i64 %8, ptr %4, align 8
  %9 = load i64, ptr %4, align 8
  call void @uart_puts(ptr noundef @.str.6) #3
  call void @uart_puts(ptr noundef @.str.7) #3
  %10 = load ptr, ptr %3, align 8
  store volatile i64 1, ptr %10, align 8
  call void @uart_puts(ptr noundef @.str.8) #3
  call void @tca_clear()
  br label %11

11:                                               ; preds = %11, %0
  call void asm sideeffect "wfi", ""() #2, !srcloc !12
  br label %11
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
  br label %3, !llvm.loop !13

11:                                               ; preds = %3
  ret void
}

; Unknown intrinsic
declare void @llvm.telos.intent.start(ptr noundef) #1

; Unknown intrinsic
declare void @llvm.telos.intent.end() #1

; Function Attrs: noinline nounwind optnone
define internal void @uart_init() #0 {
  ret void
}

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
!10 = !{i64 2147533150}
!11 = !{i64 2147533243}
!12 = !{i64 3283}
!13 = distinct !{!13, !14}
!14 = !{!"llvm.loop.mustprogress"}
