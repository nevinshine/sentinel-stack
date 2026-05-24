package integration_test

import (
	"encoding/json"
	"fmt"
	"math"
	"os"
	"os/exec"
	"path/filepath"
	"sort"
	"testing"
	"time"

	"github.com/cilium/ebpf"
)

func TestBenchmarkActiveExfiltrationGuillotine(t *testing.T) {
	vmiAlertMapPath := "/sys/fs/bpf/telos/vmi_alert_map"
	blocklistMapPath := "/sys/fs/bpf/telos/hyperion_blocklist"

	if _, err := os.Stat(vmiAlertMapPath); os.IsNotExist(err) {
		t.Fatalf("Test environment not initialized: Start telos_daemon and hyperion_ctrl before running tests.")
	}

	// 1. Build and run the dummy exfil binary
	dummyBin := filepath.Join(t.TempDir(), "exfil_dummy")
	cmd := exec.Command("go", "build", "-o", dummyBin, "./testdata/exfil_dummy.go")
	if err := cmd.Run(); err != nil {
		t.Fatalf("Failed to build dummy exfil binary: %v", err)
	}

	dummyCmd := exec.Command(dummyBin)
	if err := dummyCmd.Start(); err != nil {
		t.Fatalf("Failed to start dummy process: %v", err)
	}
	defer dummyCmd.Process.Kill()
	
	// Give it a moment to connect
	time.Sleep(500 * time.Millisecond)
	pid := uint32(dummyCmd.Process.Pid)

	// Open maps
	vmiMap, err := ebpf.LoadPinnedMap(vmiAlertMapPath, nil)
	if err != nil {
		t.Fatalf("Failed to load vmi_alert_map: %v", err)
	}
	defer vmiMap.Close()

	blocklistMap, err := ebpf.LoadPinnedMap(blocklistMapPath, nil)
	if err != nil {
		t.Fatalf("Failed to load hyperion_blocklist: %v", err)
	}
	defer blocklistMap.Close()

	var threatLevel uint32 = 2 // VMI_THREAT_MALICIOUS
	key := LPMKey{PrefixLen: 32, IP: 0x01010101}
	var val uint8

	const warmupIters = 50
	const benchmarkIters = 1000
	var latencies []time.Duration

	t.Logf("Starting benchmark... (Warmup: %d iters, Benchmark: %d iters)", warmupIters, benchmarkIters)

	for i := 0; i < warmupIters+benchmarkIters; i++ {
		// Clean maps
		_ = vmiMap.Delete(&pid)
		_ = blocklistMap.Delete(&key)
		
		// Small sleep to ensure daemon cleans state and to prevent event overlap. 
		// Use 5ms to comfortably exceed the 1ms polling interval.
		time.Sleep(5 * time.Millisecond)
		
		start := time.Now()
		
		// Inject trap
		if err := vmiMap.Put(&pid, &threatLevel); err != nil {
			t.Fatalf("Failed to inject PID: %v", err)
		}

		// Busy wait for translation
		found := false
		for j := 0; j < 50000; j++ { // ~500ms max timeout using 10us sleep
			if err := blocklistMap.Lookup(&key, &val); err == nil && val == 1 {
				found = true
				break
			}
			time.Sleep(10 * time.Microsecond)
		}
		
		duration := time.Since(start)

		if !found {
			t.Fatalf("Iteration %d timed out: telos_daemon failed to slam 1.1.1.1", i)
		}

		if i >= warmupIters {
			latencies = append(latencies, duration)
		}
	}

	// Calculate statistics
	sort.Slice(latencies, func(i, j int) bool { return latencies[i] < latencies[j] })

	var sum time.Duration
	for _, d := range latencies {
		sum += d
	}
	mean := time.Duration(int64(sum) / int64(len(latencies)))
	
	// Standard deviation
	var varianceSum float64
	meanFloat := float64(mean.Microseconds())
	for _, d := range latencies {
		diff := float64(d.Microseconds()) - meanFloat
		varianceSum += diff * diff
	}
	stdDev := time.Duration(math.Sqrt(varianceSum/float64(len(latencies)))) * time.Microsecond

	p50 := latencies[int(float64(len(latencies))*0.50)]
	p95 := latencies[int(float64(len(latencies))*0.95)]
	p99 := latencies[int(float64(len(latencies))*0.99)]
	p999 := latencies[int(float64(len(latencies))*0.999)]
	max := latencies[len(latencies)-1]

	// Clean up after run
	_ = vmiMap.Delete(&pid)
	_ = blocklistMap.Delete(&key)

	fmt.Println("\n==========================================")
	fmt.Println(" Active Exfiltration Guillotine Benchmark ")
	fmt.Println("==========================================")
	fmt.Printf(" Iterations : %d\n", benchmarkIters)
	fmt.Printf(" Mean       : %v\n", mean)
	fmt.Printf(" StdDev (σ) : %v\n", stdDev)
	fmt.Printf(" p50        : %v\n", p50)
	fmt.Printf(" p95        : %v\n", p95)
	fmt.Printf(" p99        : %v\n", p99)
	fmt.Printf(" p99.9      : %v\n", p999)
	fmt.Printf(" Max        : %v\n", max)
	// Dump raw float latencies (ms) to JSON for CDF plotting
	var floatLatencies []float64
	for _, d := range latencies {
		floatLatencies = append(floatLatencies, float64(d.Microseconds())/1000.0)
	}
	
	jsonData, err := json.Marshal(floatLatencies)
	if err == nil {
		_ = os.WriteFile("latencies.json", jsonData, 0644)
		fmt.Println(" [[PASS]] Dumped raw latencies to latencies.json for CDF plotting")
	}

	fmt.Println("==========================================")
}
