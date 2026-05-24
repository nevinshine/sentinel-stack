#!/bin/bash
set -e

echo "=========================================="
echo " Starting Guillotine Benchmark Suite"
echo "=========================================="

# Build the daemons
echo "[1] Building daemons..."
cd /home/nevin/sentinel-stack/telos-runtime && make
cd /home/nevin/sentinel-stack/hyperion-xdp && make build

# Stop existing instances if any
sudo pkill -9 telos_daemon || true
sudo pkill -9 hyperion_ctrl || true

echo "[2] Starting Hyperion XDP Engine in background..."
cd /home/nevin/sentinel-stack/hyperion-xdp
sudo ./bin/hyperion_ctrl &
HYP_PID=$!

echo "[3] Starting Telos Daemon in background (with 1ms poll interval)..."
cd /home/nevin/sentinel-stack/telos-runtime
export TELOS_POLL_INTERVAL_MS=1
sudo -E ./bin/telos_daemon &
TEL_PID=$!

# Wait for them to spin up and create pins
echo "[4] Waiting 5 seconds for BPF Maps to mount and caches to warm..."
sleep 5

echo "[5] Running Micro-Benchmark..."
cd /home/nevin/sentinel-stack/tests/integration
sudo go test -v -run TestBenchmarkActiveExfiltrationGuillotine ./...

echo "=========================================="
echo " Benchmark Complete. Tearing down Daemons..."
echo "=========================================="
sudo kill -9 $HYP_PID || true
sudo kill -9 $TEL_PID || true
sudo pkill -9 telos_daemon || true
sudo pkill -9 hyperion_ctrl || true
