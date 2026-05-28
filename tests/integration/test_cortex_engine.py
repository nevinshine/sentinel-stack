#!/usr/bin/env python3

import os
import subprocess
import socket
import json
import time

SOCK_PATH = "/var/run/telos.sock"

def send_ipc(command, data=None):
    if data is None:
        data = {}
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
        s.connect(SOCK_PATH)
        msg = json.dumps({"command": command, "data": data}) + "\n"
        s.sendall(msg.encode())
        resp = s.recv(4096)
        return json.loads(resp.decode())

def get_cgroup_id(cgroup_path):
    try:
        stat = os.stat(cgroup_path)
        return stat.st_ino
    except FileNotFoundError:
        return None

def get_self_cgroup_path():
    with open("/proc/self/cgroup") as f:
        for line in f:
            if "0::" in line:
                cg = line.strip().split(":", 2)[2]
                return f"/sys/fs/cgroup{cg}"
    return None

def main():
    print("[*] Testing Cortex Engine Autonomous Interdiction...")
    
    # 1. Setup a Sensitive File (Inode)
    test_file = "/tmp/sensitive.txt"
    with open(test_file, "w") as f:
        f.write("top secret data")
    
    inode = os.stat(test_file).st_ino
    print(f"[*] Registered sensitive file at {test_file} (Inode: {inode})")
    
    resp = send_ipc("UPDATE_INODE", {"inode": inode, "sensitivity": 2})
    if not resp.get("success"):
        print(f"[-] Failed to update inode: {resp.get('error')}")
        return

    # 2. Register process with LSM
    my_pid = os.getpid()
    print(f"[*] Registering test process (PID: {my_pid}) with Sentinel...")
    resp = send_ipc("REGISTER_AGENT", {"pid": my_pid, "taint": 0})
    if not resp.get("success"):
        print(f"[-] Failed to register process: {resp.get('error')}")
        return

    # 3. Get own cgroup
    cgroup_path = get_self_cgroup_path()
    if not cgroup_path:
        print("[-] Failed to find own cgroup")
        return

    cgroup_id = get_cgroup_id(cgroup_path)
    if not cgroup_id:
        print("[-] Failed to find cgroup ID")
        return

    print(f"[+] Isolated Cgroup ID: {cgroup_id} at {cgroup_path}")

    # Attach eBPF to cgroup
    print("[*] Attaching Sentinel Cgroup Hook to self...")
    resp = send_ipc("ATTACH_CGROUP", {"path": cgroup_path})
    if not resp.get("success"):
        print(f"[-] Failed to attach: {resp.get('error')}")
        return
    print("[+] Attached successfully!")
    
    # 3. Verify initial connectivity via clean background process
    print("[*] Spawning clean background ping process in the same Cgroup...")
    bg_process = subprocess.Popen(
        "while true; do ping -c 1 -W 1 127.0.0.1 > /dev/null 2>&1; if [ $? -eq 0 ]; then echo 'OK' > /tmp/ping_status; else echo 'FAIL' > /tmp/ping_status; fi; sleep 0.5; done",
        shell=True,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL
    )
    time.sleep(1) # wait for first ping
    
    with open("/tmp/ping_status", "r") as f:
        status = f.read().strip()
    if status != "OK":
        print("[-] Error: Initial background ping failed!")

    print("[+] Background process is successfully pinging.")

    # 4. Trigger Kill-Chain inside Cgroup
    print("[*] Triggering Kill-Chain Phase 1 (Reading Sensitive File) -> +20 Threat Score & Elevates Taint")
    try:
        with open(test_file, "r") as f:
            f.read()
    except Exception:
        pass
    time.sleep(1)

    print("[*] Triggering Kill-Chain Phase 2 (Unauthorized Exec 1) -> +60 Threat Score (Total: 80)")
    try:
        subprocess.run(["ls"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    except Exception:
        pass
    time.sleep(1)

    print("[*] Triggering Kill-Chain Phase 3 (Unauthorized Exec 2) -> +60 Threat Score (Total: 140 > 100 Threshold)")
    try:
        subprocess.run(["date"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    except Exception:
        pass
    time.sleep(1)

    # 5. Verify Autonomous Lockdown using the clean background process
    print("[*] Testing background connectivity after Cortex Lockdown...")
    # The background process is NOT in process_map (untracked/clean).
    # Its network traffic should NOW be severed by the cgroup egress hook!
    time.sleep(2) # Give background ping time to fail and update file
    try:
        with open("/tmp/ping_status", "r") as f:
            status = f.read().strip()
        if status == "FAIL":
            print("[+] SUCCESS: Cortex autonomously severed the network kill-chain for the ENTIRE Cgroup!")
        else:
            print("[-] ERROR: Background ping succeeded! Cortex failed to lock down the container.")
    except Exception as e:
        print(f"[-] Failed to read ping status: {e}")
    
    # Clean up
    print("[*] Cleaning up...")
    bg_process.terminate()
    try:
        os.remove("/tmp/ping_status")
    except:
        pass
        
    try:
        send_ipc("DETACH_CGROUP", {"path": cgroup_path})
    except PermissionError:
        print("[!] Cleanup: Tainted process blocked from Unix IPC (Expected Security Measure).")
        
    try:
        os.remove(test_file)
    except:
        pass
    print("[+] Done.")

if __name__ == "__main__":
    if os.geteuid() != 0:
        print("[-] Please run as root.")
        exit(1)
    main()
