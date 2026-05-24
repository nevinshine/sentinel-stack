package integration_test

import (
	"bufio"
	"os"
	"os/exec"
	"path/filepath"
	"syscall"
	"testing"
	"time"

	"github.com/cilium/ebpf"
)

type LPMKey struct {
	PrefixLen uint32
	IP        uint32
}

func TestActiveExfiltrationGuillotine(t *testing.T) {
	vmiAlertMapPath := "/sys/fs/bpf/telos/vmi_alert_map"
	blocklistMapPath := "/sys/fs/bpf/telos/hyperion_blocklist"

	if _, err := os.Stat(vmiAlertMapPath); os.IsNotExist(err) {
		t.Fatalf("Test environment not initialized: Start telos_daemon and hyperion_ctrl before running tests.")
	}
	if _, err := os.Stat(blocklistMapPath); os.IsNotExist(err) {
		t.Fatalf("Test environment not initialized: Start telos_daemon and hyperion_ctrl before running tests.")
	}

	// 1. Build the dummy exfil binary
	dummyBin := filepath.Join(t.TempDir(), "exfil_dummy")
	cmd := exec.Command("go", "build", "-o", dummyBin, "./testdata/exfil_dummy.go")
	if err := cmd.Run(); err != nil {
		t.Fatalf("Failed to build dummy exfil binary: %v", err)
	}

	// 2. Start the dummy process
	dummyCmd := exec.Command(dummyBin)
	stdout, err := dummyCmd.StdoutPipe()
	if err != nil {
		t.Fatalf("Failed to get stdout pipe: %v", err)
	}
	
	if err := dummyCmd.Start(); err != nil {
		t.Fatalf("Failed to start dummy process: %v", err)
	}

	// Wait for dummy to signal it is READY
	scanner := bufio.NewScanner(stdout)
	ready := make(chan bool)
	go func() {
		for scanner.Scan() {
			if scanner.Text() == "READY" {
				ready <- true
				break
			}
		}
	}()

	select {
	case <-ready:
		t.Logf("Dummy process started with PID: %d and connection established", dummyCmd.Process.Pid)
	case <-time.After(5 * time.Second):
		dummyCmd.Process.Kill()
		t.Fatalf("Timeout waiting for dummy process to become ready")
	}

	pid := uint32(dummyCmd.Process.Pid)

	// Setup Map Teardown Defer
	defer func() {
		// Clean up the alert map
		vmiMap, err := ebpf.LoadPinnedMap(vmiAlertMapPath, nil)
		if err == nil {
			vmiMap.Delete(&pid)
			vmiMap.Close()
		}

		// Clean up the blocklist map (1.1.1.1 = 0x01010101)
		blocklistMap, err := ebpf.LoadPinnedMap(blocklistMapPath, nil)
		if err == nil {
			key := LPMKey{PrefixLen: 32, IP: 0x01010101}
			blocklistMap.Delete(&key)
			blocklistMap.Close()
		}
		
		dummyCmd.Process.Kill()
	}()

	// 3. Inject PID into vmi_alert_map
	vmiMap, err := ebpf.LoadPinnedMap(vmiAlertMapPath, nil)
	if err != nil {
		t.Fatalf("Failed to load vmi_alert_map: %v", err)
	}
	
	var threatLevel uint32 = 2 // VMI_THREAT_MALICIOUS
	if err := vmiMap.Put(&pid, &threatLevel); err != nil {
		vmiMap.Close()
		t.Fatalf("Failed to inject PID into vmi_alert_map: %v", err)
	}
	vmiMap.Close()
	t.Logf("Injected PID %d into vmi_alert_map with Threat Level 2", pid)

	// 4. Wait for telos_daemon to detect and translate
	// Check for up to 3 seconds for the blocklist map to be updated
	blocklistMap, err := ebpf.LoadPinnedMap(blocklistMapPath, nil)
	if err != nil {
		t.Fatalf("Failed to load hyperion_blocklist: %v", err)
	}
	defer blocklistMap.Close()

	key := LPMKey{PrefixLen: 32, IP: 0x01010101}
	var val uint8
	
	timeout := time.After(3 * time.Second)
	ticker := time.NewTicker(200 * time.Millisecond)
	defer ticker.Stop()

	foundInBlocklist := false
verifyLoop:
	for {
		select {
		case <-timeout:
			break verifyLoop
		case <-ticker.C:
			if err := blocklistMap.Lookup(&key, &val); err == nil && val == 1 {
				foundInBlocklist = true
				break verifyLoop
			}
		}
	}

	if !foundInBlocklist {
		t.Fatalf("Timeout: telos_daemon did not inject 1.1.1.1 into hyperion_blocklist")
	}
	t.Log("Successfully verified 1.1.1.1 in hyperion_blocklist!")

	// 5. Send SIGUSR1 to dummy to trigger second connection (LSM verification)
	t.Log("Sending SIGUSR1 to dummy process...")
	if err := dummyCmd.Process.Signal(syscall.SIGUSR1); err != nil {
		t.Fatalf("Failed to send SIGUSR1 to dummy: %v", err)
	}

	// 6. Wait for dummy to exit and check exit status
	err = dummyCmd.Wait()
	if err != nil {
		if exitErr, ok := err.(*exec.ExitError); ok {
			if status, ok := exitErr.Sys().(syscall.WaitStatus); ok {
				t.Fatalf("Dummy process failed with exit status %d (Expected 0 for EPERM)", status.ExitStatus())
			}
		}
		t.Fatalf("Dummy process failed to wait: %v", err)
	}
	
	t.Log("Dummy process exited cleanly with status 0! LSM successfully blocked the connection with EPERM.")
}
