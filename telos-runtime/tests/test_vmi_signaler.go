package main

import (
	"errors"
	"fmt"
	"net"
	"os"
	"syscall"
	"time"

	"github.com/cilium/ebpf"
)

const (
	VmiAlertMapPath    = "/sys/fs/bpf/vmi_alert_map"
	VmiThreatMalicious = 2
	VmiThreatClean     = 0
)

func main() {
	fmt.Println("==================================================================")
	fmt.Println("=== STARTING PILLAR 4: CROSS-LAYER INTERCEPT INTEGRATION TEST  ===")
	fmt.Println("==================================================================")

	// Step 1: Open the pinned vmi_alert_map
	fmt.Printf("[Test] Opening pinned vmi_alert_map at %s...\n", VmiAlertMapPath)
	vmiMap, err := ebpf.LoadPinnedMap(VmiAlertMapPath, nil)
	if err != nil {
		fmt.Printf("[Test] FATAL: Failed to load pinned map. Is telos-daemon loaded? %v\n", err)
		os.Exit(1)
	}
	defer vmiMap.Close()
	fmt.Println("[Test] [PASS] Map loaded successfully!")

	// Step 2: Query our current PID
	myPid := uint32(os.Getpid())
	fmt.Printf("[Test] Current test runner PID: %d\n", myPid)

	// Verify that we can write to the map
	fmt.Printf("[Test] Quarantining current PID %d inside vmi_alert_map...\n", myPid)
	threatLevel := uint32(VmiThreatMalicious)
	err = vmiMap.Put(&myPid, &threatLevel)
	if err != nil {
		fmt.Printf("[Test] FATAL: Failed to write quarantine record: %v\n", err)
		os.Exit(1)
	}
	fmt.Println("[Test] [PASS] PID successfully quarantined under VMI_THREAT_MALICIOUS (2)")

	// Step 3: Attempt a socket connection and assert immediate EPERM
	fmt.Println("[Test] Attempting socket connection while quarantined...")
	conn, dialErr := net.DialTimeout("tcp", "127.0.0.1:54321", 500*time.Millisecond)
	if conn != nil {
		conn.Close()
		fmt.Println("[Test] ✗ FAILURE: Socket connection succeeded! Interception bypassed!")
		cleanupAndExit(vmiMap, myPid, 1)
	}

	if dialErr == nil {
		fmt.Println("[Test] ✗ FAILURE: No error returned! Interception bypassed!")
		cleanupAndExit(vmiMap, myPid, 1)
	}

	fmt.Printf("[Test] Received connection error: %v\n", dialErr)

	// Assert the error is "permission denied" (syscall.EPERM)
	var opErr *net.OpError
	var sysErr syscall.Errno
	isEperm := false

	if errors.As(dialErr, &opErr) {
		if errors.As(opErr.Err, &sysErr) {
			if sysErr == syscall.EPERM {
				isEperm = true
			}
		}
	}

	if isEperm {
		fmt.Println("[Test] [PASS] SUCCESS: Connect attempt blocked with -EPERM (Permission Denied)!")
	} else {
		fmt.Println("[Test] ✗ FAILURE: Connect failed but not with -EPERM (was it connection refused?)")
		cleanupAndExit(vmiMap, myPid, 1)
	}

	// Step 4: Lift the quarantine and assert the connect block is removed
	fmt.Printf("[Test] Lifting quarantine for PID %d...\n", myPid)
	err = vmiMap.Delete(&myPid)
	if err != nil {
		fmt.Printf("[Test] Failed to delete quarantine entry: %v\n", err)
	}

	fmt.Println("[Test] Attempting connection after lifting quarantine...")
	conn2, dialErr2 := net.DialTimeout("tcp", "127.0.0.1:54321", 500*time.Millisecond)
	if conn2 != nil {
		conn2.Close()
	}

	isEpermAfter := false
	if dialErr2 != nil {
		if errors.As(dialErr2, &opErr) {
			if errors.As(opErr.Err, &sysErr) {
				if sysErr == syscall.EPERM {
					isEpermAfter = true
				}
			}
		}
	}

	if isEpermAfter {
		fmt.Println("[Test] ✗ FAILURE: Connection still blocked with EPERM after lifting quarantine!")
		cleanupAndExit(vmiMap, myPid, 1)
	} else {
		fmt.Println("[Test] [PASS] SUCCESS: Connection allowed (no EPERM block active) after lifting quarantine!")
	}

	fmt.Println("\n==================================================================")
	fmt.Println("=== [SUCCESS] PILLAR 4 CROSS-LAYER LOCKDOWN VERIFIED GATES     ===")
	fmt.Println("==================================================================")
	cleanupAndExit(vmiMap, myPid, 0)
}

func cleanupAndExit(vmiMap *ebpf.Map, pid uint32, exitCode int) {
	_ = vmiMap.Delete(&pid)
	os.Exit(exitCode)
}
