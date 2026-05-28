import subprocess
import os

RESULTS_DIR = "tests/benchmarks/results"
TELEMETRY_CSV = f"{RESULTS_DIR}/telemetry_saturation.csv"
OVERHEAD_CSV = f"{RESULTS_DIR}/daemon_overhead.csv"

RUNNER_SCRIPT = """#!/bin/bash
# Local extraction harness to avoid SSH drops during network saturation
set -e

TELEMETRY_CSV="/tmp/telemetry_saturation.csv"
OVERHEAD_CSV="/tmp/daemon_overhead.csv"

echo "target_flood,captured_blocks" > $TELEMETRY_CSV
echo "target_flood,usr_cpu,sys_cpu,rss_kb" > $OVERHEAD_CSV

for flood_count in {1000..50000..5000}; do
    echo "[*] Testing flood count: $flood_count..."
    
    # Get before metric
    METRIC_BEFORE=$(curl -s http://localhost:9094/metrics | grep "^sentinel_network_blocks_total" | awk '{print $2}')
    if [ -z "$METRIC_BEFORE" ]; then METRIC_BEFORE=0; fi
    
    # Start pidstat
    pidstat -p $(pidof telos_daemon) -r -u 2 1 > /tmp/pidstat.out &
    PIDSTAT_PID=$!
    
    # Run strike
    sudo timeout 10 sentinel_strike_ring0 $flood_count || true
    
    # Wait for pidstat
    wait $PIDSTAT_PID 2>/dev/null || true
    
    # Get after metric
    METRIC_AFTER=$(curl -s http://localhost:9094/metrics | grep "^sentinel_network_blocks_total" | awk '{print $2}')
    if [ -z "$METRIC_AFTER" ]; then METRIC_AFTER=0; fi
    
    DELTA=$((METRIC_AFTER - METRIC_BEFORE))
    echo "$flood_count,$DELTA" >> $TELEMETRY_CSV
    echo "    Captured Blocks: $DELTA"
    
    # Parse pidstat
    # Format typically contains %usr %system ... VSZ RSS ...
    # We will just grep for telos_daemon and average
    AVGS=$(grep telos_daemon /tmp/pidstat.out | awk '{u+=$4; s+=$5; r+=$12; c++} END {if(c>0) print u/c, s/c, r/c; else print 0,0,0}')
    
    # if empty string, set 0 0 0
    if [ -z "$AVGS" ]; then AVGS="0 0 0"; fi
    
    # Replace space with comma
    AVGS_CSV=$(echo $AVGS | tr ' ' ',')
    echo "$flood_count,$AVGS_CSV" >> $OVERHEAD_CSV
    echo "    Overhead: $AVGS_CSV"
    
    sleep 1
done
"""

def run_benchmark():
    os.makedirs(RESULTS_DIR, exist_ok=True)
    
    print("[*] Generating local runner script...")
    with open("/tmp/runner.sh", "w") as f:
        f.write(RUNNER_SCRIPT)
    
    print("[*] Uploading runner script to intelhost...")
    # Using vagrant ssh-config to scp is annoying, let's just pipe it via ssh
    subprocess.run(["vagrant", "ssh", "intelhost", "-c", f"cat > /tmp/runner.sh"], input=RUNNER_SCRIPT, text=True, check=True)
    subprocess.run(["vagrant", "ssh", "intelhost", "-c", "chmod +x /tmp/runner.sh"], check=True)
    
    print("[*] Executing Benchmark Suite on intelhost (This will take a few minutes)...")
    subprocess.run(["vagrant", "ssh", "intelhost", "-c", "/tmp/runner.sh"], check=True)
    
    print("[*] Downloading CSV results...")
    # Extract via cat
    tel_csv = subprocess.run(["vagrant", "ssh", "intelhost", "-c", "cat /tmp/telemetry_saturation.csv"], stdout=subprocess.PIPE, text=True, check=True).stdout
    ov_csv = subprocess.run(["vagrant", "ssh", "intelhost", "-c", "cat /tmp/daemon_overhead.csv"], stdout=subprocess.PIPE, text=True, check=True).stdout
    
    with open(TELEMETRY_CSV, "w") as f:
        f.write(tel_csv)
    with open(OVERHEAD_CSV, "w") as f:
        f.write(ov_csv)
        
    print("[*] Extraction complete.")

if __name__ == "__main__":
    run_benchmark()
