package main

/*
#include <stdint.h>
void heki_intent_unlock(uint64_t nonce) {
    uint32_t magic = 0x48454B49; // "HEKI"
    uint32_t eax = magic;
    uint32_t ecx = (uint32_t)(nonce & 0xFFFFFFFF);
    uint32_t ebx = (uint32_t)(nonce >> 32);
    uint32_t edx = 0;

    // Execute CPUID to trigger VMEXIT.
    // KVM will intercept this and sentinel-vmi will catch the KVMI_EVENT_CPUID.
    asm volatile(
        "cpuid"
        : "+a"(eax), "+b"(ebx), "+c"(ecx), "+d"(edx)
        :
        : "memory"
    );
}
*/
import "C"
import (
	"bytes"
	"encoding/binary"
	"net"
	"os"
)

// hekiNonce is established during the initial IPC registration with sentinel-vmi
var hekiNonce uint64 = 0

// HekiIntentUnlock executes an authenticated CPUID instruction to signal
// to the sentinel-vmi hypervisor that the Go daemon intends to write
// to a protected map. The hypervisor will authorize the current vCPU's CR3.
func HekiIntentUnlock() {
	if hekiNonce != 0 {
		C.heki_intent_unlock(C.uint64_t(hekiNonce))

		// MOCK VMEXIT for demonstration: Send CPUID intent via IPC
		// In a real environment, KVM intercepts the CPUID assembly above!
		hekiSocket := os.Getenv("TELOS_HEKI_VMI_SOCKET")
		if hekiSocket == "" {
			hekiSocket = "/tmp/heki.sock"
		}
		conn, err := net.Dial("unix", hekiSocket)
		if err == nil {
			defer conn.Close()
			req := HekiDrawbridgeRequest{
				PayloadGva:      0,
				PayloadSize:     0,
				TargetSubsystem: 1, // HEKI_SUBSYSTEM_TELOS
				EphemeralNonce:  hekiNonce,
				Magic:           0x4D4F434B, // "MOCK"
			}
			buf := new(bytes.Buffer)
			binary.Write(buf, binary.LittleEndian, &req)
			conn.Write(buf.Bytes())

			ack := make([]byte, 8)
			conn.Read(ack)
		}
	}
}
