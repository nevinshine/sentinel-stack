#!/bin/bash
set -e

echo "=================================================="
echo " Sentinel Stack: Cross-Layer Enforcement Parity "
echo "=================================================="

# Test IP: 198.51.100.99 (TEST-NET-2)
# Hex encoding for IPv4 (Network Byte Order): c6 33 64 63
IP_KEY="c6 33 64 63"
BLOCK_VAL="01 00 00 00"
TEST_IP="198.51.100.99"

# Clean up any previous instances
sudo pkill -f hyperion_ctrl || true
sudo pkill -f loader || true
sudo rm -f dummy
sleep 1

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

# 1. Start Hyperion XDP on loopback
echo "[1] Starting Hyperion XDP on 'lo'..."
cd "$REPO_ROOT/hyperion-xdp"
make build >/dev/null
sudo ./bin/hyperion_ctrl -iface lo &
XDP_PID=$!
sleep 2 # wait for XDP to attach

# 2. Start Sentinel-CC LSM
echo "[2] Starting Sentinel-CC LSM Hook..."
cd "$REPO_ROOT/sentinel-cc"
echo "int main() { sleep(3600); return 0; }" > dummy.c
gcc dummy.c -o dummy
sudo ./loader --audit ./dummy &
LSM_PID=$!
sleep 2 # wait for LSM to load

# 3. Inject policy into all blocklist maps (XDP and LSM)
echo "[3] Injecting test IP ($TEST_IP) into blocklist maps via bpftool..."
for id in $(sudo bpftool map | grep blocklist_map | awk '{print $1}' | tr -d ':'); do
    echo " -> Updating map ID $id"
    sudo bpftool map update id $id key hex $IP_KEY value hex $BLOCK_VAL
done

echo ""
echo "--- BEGIN LAYER 7 (LSM) TEST ---"
# Test A: The LSM Check
# We expect `nc` to call connect(), which should be intercepted and denied by LSM (-EPERM).
set +e
NC_OUTPUT=$(nc -w 1 $TEST_IP 80 2>&1)
NC_EXIT=$?
set -e

echo "nc output: $NC_OUTPUT"
if echo "$NC_OUTPUT" | grep -qi "Permission denied"; then
    echo "✓ [PASS] Layer 7 LSM blocked the connect() syscall."
else
    echo "✗ [FAIL] Layer 7 LSM did NOT block connect() with Permission denied."
    sudo kill $XDP_PID $LSM_PID || true
    exit 1
fi

echo ""
echo "--- BEGIN LAYER 2 (XDP) TEST ---"
# Test B: The XDP Check
# We expect `ping` (which doesn't always use connect) to be silently dropped by XDP at ingress/egress.
# Since we attached to 'lo', we route the IP to lo to force it through the XDP hook.
sudo ip route add $TEST_IP dev lo || true

set +e
PING_OUTPUT=$(ping -c 1 -W 1 $TEST_IP 2>&1)
PING_EXIT=$?
set -e

sudo ip route del $TEST_IP dev lo || true

echo "ping output:"
echo "$PING_OUTPUT"
if echo "$PING_OUTPUT" | grep -q "100% packet loss"; then
    echo "✓ [PASS] Layer 2 XDP silently dropped the packet (100% loss)."
else
    echo "✗ [FAIL] Layer 2 XDP did NOT drop the packet."
    sudo kill $XDP_PID $LSM_PID || true
    exit 1
fi

# Cleanup
echo "[4] Cleaning up..."
sudo kill $XDP_PID $LSM_PID || true
wait $XDP_PID 2>/dev/null || true
wait $LSM_PID 2>/dev/null || true

echo "=================================================="
echo " ✓ ALL CROSS-LAYER PARITY TESTS PASSED"
echo "=================================================="
exit 0
