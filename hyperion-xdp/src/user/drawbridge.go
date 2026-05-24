package main

/*
#include <stdint.h>
static inline void drawbridge_hypercall(uint64_t nonce) {
    uint32_t eax = 0x48454B49; // "HEKI"
    uint32_t ecx = (uint32_t)(nonce & 0xFFFFFFFF);
    uint32_t ebx = (uint32_t)(nonce >> 32);
    // Unprivileged backdoor hypercall
    __asm__ __volatile__(
        "cpuid"
        : "+a"(eax), "+c"(ecx), "+b"(ebx)
        :
        : "edx"
    );
}
*/
import "C"

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"log"
	"net"
	"os"
	"time"

	"github.com/cilium/ebpf"
)

// LPMKey must match the struct lpm_ip_key in hyperion_core.c
type LPMKey struct {
	PrefixLen uint32
	IP        uint32
}

// ExecuteDrawbridgeUpdate chunks updates into blocks of 256 and executes Drawbridge sequence.
func ExecuteDrawbridgeUpdate(nonce uint64, batchKeys []LPMKey, batchValues []uint8, m *ebpf.Map) error {
	total := len(batchKeys)
	if total == 0 {
		return nil
	}

	const chunkSize = 256
	log.Printf("Executing Drawbridge Update: %d IPs total, %d max per batch", total, chunkSize)

	for i := 0; i < total; i += chunkSize {
		end := i + chunkSize
		if end > total {
			end = total
		}

		keys := batchKeys[i:end]
		values := batchValues[i:end]

		// Phase 1: Intent Registration
		// The hypervisor traps this CPUID, verifies the nonce, and readies the NPT unlock
		C.drawbridge_hypercall(C.uint64_t(nonce))

		// MOCK VMEXIT for demonstration: Send CPUID intent via IPC
		hekiSocket := os.Getenv("TELOS_HEKI_VMI_SOCKET")
		if hekiSocket == "" {
			hekiSocket = "/tmp/heki.sock"
		}
		conn, errDial := net.Dial("unix", hekiSocket)
		if errDial == nil {
			req := HekiDrawbridgeRequest{
				PayloadGva:      0,
				PayloadSize:     0,
				TargetSubsystem: 2, // HEKI_SUBSYSTEM_HYPERION
				EphemeralNonce:  nonce,
				Magic:           0x4D4F434B, // "MOCK"
			}
			buf := new(bytes.Buffer)
			binary.Write(buf, binary.LittleEndian, &req)
			conn.Write(buf.Bytes())
			ack := make([]byte, 8)
			conn.Read(ack)
			conn.Close()
		}

		// Phase 2: Amortized Batch Update
		// The hypervisor will MTF single-step the internal kernel writes and re-lock
		_, err := m.BatchUpdate(keys, values, &ebpf.BatchOptions{})
		if err != nil {
			return fmt.Errorf("batch update failed at chunk %d-%d: %w", i, end, err)
		}

		log.Printf("[Drawbridge] Successfully applied chunk %d-%d", i, end)

		// Yield slightly to prevent any potential hypervisor starvation during massive dumps
		if end < total {
			time.Sleep(1 * time.Microsecond)
		}
	}

	return nil
}
