#!/usr/bin/env python3

import os
import subprocess
import socket
import json
import time

SOCK_PATH = "/var/run/telos.sock"

def send_ipc(command, data):
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
        s.connect(SOCK_PATH)
        msg = json.dumps({"command": command, "data": data}) + "\n"
        s.sendall(msg.encode())
        resp = s.recv(4096)
        return json.loads(resp.decode())

def get_cgroup_id(cgroup_path):
    # Retrieve the cgroup inode ID which is used by eBPF bpf_get_current_cgroup_id
    try:
        stat = os.stat(cgroup_path)
        return stat.st_ino
    except FileNotFoundError:
        return None

def main():
    print("[*] Setting up isolated cgroup via systemd-run...")
    # Create a transient systemd scope for testing
    cgroup_name = "sentinel-cgroup-test.service"
    subprocess.run(["systemd-run", "--unit", cgroup_name, "sleep", "1000"], check=True)
    
    cgroup_path = f"/sys/fs/cgroup/system.slice/{cgroup_name}"
    
    # Wait for cgroup to be created
    time.sleep(1)
    
    cgroup_id = get_cgroup_id(cgroup_path)
    if not cgroup_id:
        print("[-] Failed to find cgroup ID")
        return

    print(f"[+] Isolated Cgroup ID: {cgroup_id} at {cgroup_path}")

    # Attach eBPF to cgroup
    print("[*] Attaching Sentinel Cgroup Hook...")
    resp = send_ipc("ATTACH_CGROUP", {"path": cgroup_path})
    if not resp.get("success"):
        print(f"[-] Failed to attach: {resp.get('error')}")
        return
    print("[+] Attached successfully!")

    # Block 8.8.8.8 for this cgroup
    target_ip = "8.8.8.8"
    print(f"[*] Blocking {target_ip} for cgroup {cgroup_id}...")
    resp = send_ipc("UPDATE_CGROUP_POLICY", {
        "cgroup_id": cgroup_id,
        "ip": target_ip,
        "allowed": 0
    })
    if not resp.get("success"):
        print(f"[-] Failed to update policy: {resp.get('error')}")
        return
    print("[+] Policy updated successfully!")

    # Test the connection from within the cgroup
    print(f"[*] Testing connection to {target_ip} from within cgroup...")
    # We can run ping inside the cgroup using systemd-run
    try:
        subprocess.run(
            ["systemd-run", "--unit", "ping-test.scope", "--slice", "system.slice", "ping", "-c", "1", "-W", "1", target_ip],
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        print(f"[-] ERROR: Ping to {target_ip} succeeded but should have been blocked by Sentinel!")
    except subprocess.CalledProcessError:
        print(f"[+] SUCCESS: Ping to {target_ip} was correctly blocked by Sentinel's cgroup_skb/egress hook!")

    # Clean up
    print("[*] Detaching Sentinel Cgroup Hook...")
    send_ipc("DETACH_CGROUP", {"path": cgroup_path})
    subprocess.run(["systemctl", "stop", cgroup_name], check=False, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    print("[+] Cleaned up test environment.")

if __name__ == "__main__":
    if os.geteuid() != 0:
        print("[-] Please run as root.")
        exit(1)
    main()
