#!/bin/bash
set -e

echo "=========================================="
echo " Starting Guillotine Integration Test"
echo "=========================================="

WORKSPACE=$(cd "$(dirname "$0")/../.." && pwd)

# Build the daemons
echo "[1] Building daemons..."
cd "$WORKSPACE/telos-runtime" && make
cd "$WORKSPACE/hyperion-xdp" && make build

# Stop existing instances if any
sudo pkill -9 telos_daemon || true
sudo pkill -9 hyperion_ctrl || true

echo "[2] Starting Hyperion XDP Engine in background..."
cd "$WORKSPACE/hyperion-xdp"
sudo ./bin/hyperion_ctrl &
HYP_PID=$!

echo "[3] Starting Telos Daemon in background..."
cd "$WORKSPACE/telos-runtime"
sudo ./bin/telos_daemon &
TEL_PID=$!

# Wait for them to spin up and create pins
echo "[4] Waiting 5 seconds for BPF Maps to mount..."
sleep 5

echo "[5] Running Capstone Test Suite..."
cd "$WORKSPACE/tests/integration"
sudo go test -v ./...

echo "=========================================="
echo " Test Complete. Tearing down Daemons..."
echo "=========================================="
sudo kill -9 $HYP_PID || true
sudo kill -9 $TEL_PID || true
sudo pkill -9 telos_daemon || true
sudo pkill -9 hyperion_ctrl || true
