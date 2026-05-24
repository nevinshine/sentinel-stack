package main

import (
	"bufio"
	"fmt"
	"log"
	"os"
	"path/filepath"
	"strconv"
	"strings"

	"github.com/cilium/ebpf"
)

// LPMKey must match struct lpm_ip_key in hyperion_core.c
type LPMKey struct {
	PrefixLen uint32
	IP        uint32
}

// EnforceNetworkSlam drops all active network connections for a malicious PID
// by enumerating its sockets and adding the destination IPs to the Hyperion XDP blocklist.
func (d *TelosDaemon) EnforceNetworkSlam(pid uint32, siemIP string) {
	log.Printf("[NetworkSlam] Initiating active containment for PID %d", pid)

	var siemIPVal uint32 = 0
	if siemIP != "" {
		// Parse SIEM IP string into uint32 (little endian representation matching /proc/net/tcp hex string)
		// e.g. 192.168.1.100 -> C0.A8.01.64 -> 0x6401A8C0
		parts := strings.Split(siemIP, ".")
		if len(parts) == 4 {
			b0, _ := strconv.Atoi(parts[0])
			b1, _ := strconv.Atoi(parts[1])
			b2, _ := strconv.Atoi(parts[2])
			b3, _ := strconv.Atoi(parts[3])
			siemIPVal = uint32(b0) | uint32(b1)<<8 | uint32(b2)<<16 | uint32(b3)<<24
		}
	}

	// 1. Find all socket inodes for the given PID
	inodes, err := getSocketInodes(pid)
	if err != nil {
		log.Printf("[NetworkSlam] Error getting socket inodes for PID %d: %v", pid, err)
		return
	}
	if len(inodes) == 0 {
		log.Printf("[NetworkSlam] No active sockets found for PID %d", pid)
		return
	}

	// 2. Resolve destination IPs from /proc/net/tcp and /proc/net/udp
	destIPs := make(map[uint32]bool)
	findDestIPs("/proc/net/tcp", inodes, destIPs)
	findDestIPs("/proc/net/udp", inodes, destIPs)

	if len(destIPs) == 0 {
		log.Printf("[NetworkSlam] No external destination IPs resolved for PID %d", pid)
		return
	}

	// 3. Load Hyperion blocklist map
	blocklistPinnedPath := "/sys/fs/bpf/telos/hyperion_blocklist"
	blocklistMap, err := ebpf.LoadPinnedMap(blocklistPinnedPath, nil)
	if err != nil {
		log.Printf("[NetworkSlam] Error loading hyperion_blocklist map: %v", err)
		return
	}
	defer blocklistMap.Close()

	// 4. Update the blocklist map
	for ip := range destIPs {
		// Skip localhost/loopback (127.x.x.x -> highest byte in uint32 is 0x7F) or 0.0.0.0
		if ip == 0 || (ip&0xFF) == 0x7F {
			continue
		}

		// Skip SIEM IP
		if ip == siemIPVal {
			continue
		}

		key := LPMKey{PrefixLen: 32, IP: ip}
		var val uint8 = 1 // 1 = blocked

		if err := blocklistMap.Put(&key, &val); err != nil {
			log.Printf("[NetworkSlam] Failed to block IP %08X: %v", ip, err)
		} else {
			log.Printf("[NetworkSlam] [PASS] Wire-speed drop engaged for IP %08X", ip)
		}
	}
}

// getSocketInodes reads /proc/<pid>/fd and extracts all socket inodes
func getSocketInodes(pid uint32) (map[string]bool, error) {
	inodes := make(map[string]bool)
	fdPath := fmt.Sprintf("/proc/%d/fd", pid)

	entries, err := os.ReadDir(fdPath)
	if err != nil {
		return nil, err
	}

	for _, entry := range entries {
		linkPath := filepath.Join(fdPath, entry.Name())
		target, err := os.Readlink(linkPath)
		if err != nil {
			continue
		}

		if strings.HasPrefix(target, "socket:[") && strings.HasSuffix(target, "]") {
			inodeStr := target[8 : len(target)-1]
			inodes[inodeStr] = true
		}
	}

	return inodes, nil
}

// findDestIPs parses a /proc/net file and finds destination IPs for given inodes
func findDestIPs(path string, inodes map[string]bool, destIPs map[uint32]bool) {
	file, err := os.Open(path)
	if err != nil {
		return
	}
	defer file.Close()

	scanner := bufio.NewScanner(file)
	// Skip header
	if scanner.Scan() {
		_ = scanner.Text()
	}

	for scanner.Scan() {
		line := scanner.Text()
		fields := strings.Fields(line)
		if len(fields) < 10 {
			continue
		}

		// fields[1] = local_address, fields[2] = rem_address, fields[9] = inode
		inode := fields[9]
		if !inodes[inode] {
			continue
		}

		remAddr := fields[2]
		parts := strings.Split(remAddr, ":")
		if len(parts) != 2 {
			continue
		}

		ipHex := parts[0]
		ipVal, err := strconv.ParseUint(ipHex, 16, 32)
		if err == nil {
			destIPs[uint32(ipVal)] = true
		}
	}
}
