package main

import (
	"crypto/ed25519"
	"debug/elf"
	"fmt"
	"os"
	"os/exec"
	"syscall"

	"github.com/cilium/ebpf"
)

type InodeKey struct {
	Dev uint64
	Ino uint64
}

const (
	PubKeyPath = "/home/nevin/.telos/pcc.pub"
	MapPath    = "/sys/fs/bpf/telos/pcc_auth_map"
)

func main() {
	if len(os.Args) != 3 || os.Args[1] != "register" {
		fmt.Println("Usage: telos_ctrl register <binary>")
		os.Exit(1)
	}

	binaryPath := os.Args[2]
	fmt.Printf("[telos_ctrl] Registering binary: %s\n", binaryPath)

	// 1. Load Public Key
	pubKeyBytes, err := os.ReadFile(PubKeyPath)
	if err != nil {
		fmt.Printf("[FAIL] Failed to read public key: %v\n", err)
		os.Exit(1)
	}
	if len(pubKeyBytes) != ed25519.PublicKeySize {
		fmt.Printf("[FAIL] Invalid public key size\n")
		os.Exit(1)
	}
	pubKey := ed25519.PublicKey(pubKeyBytes)

	// 2. Extract Signature
	f, err := elf.Open(binaryPath)
	if err != nil {
		fmt.Printf("[FAIL] Failed to open ELF: %v\n", err)
		os.Exit(1)
	}
	defer f.Close()

	section := f.Section(".telos_pcc")
	if section == nil {
		fmt.Printf("[FAIL] Binary missing .telos_pcc section. Not compiled by telos-lang?\n")
		os.Exit(1)
	}

	sigBytes, err := section.Data()
	if err != nil {
		fmt.Printf("[FAIL] Failed to read signature data: %v\n", err)
		os.Exit(1)
	}
	if len(sigBytes) != ed25519.SignatureSize {
		fmt.Printf("[FAIL] Invalid signature size in .telos_pcc\n")
		os.Exit(1)
	}

	// 3. Reconstruct Original Bytes for Verification
	tmpPath := binaryPath + ".stripped"
	cmd := exec.Command("objcopy", "--remove-section", ".telos_pcc", binaryPath, tmpPath)
	if err := cmd.Run(); err != nil {
		fmt.Printf("[FAIL] Failed to strip section for verification: %v\n", err)
		os.Exit(1)
	}
	defer os.Remove(tmpPath)

	rawBytes, err := os.ReadFile(tmpPath)
	if err != nil {
		fmt.Printf("[FAIL] Failed to read stripped binary: %v\n", err)
		os.Exit(1)
	}

	// 4. Cryptographic Verification
	if !ed25519.Verify(pubKey, rawBytes, sigBytes) {
		fmt.Printf("[FAIL] Cryptographic signature VERIFICATION FAILED. Binary tampered!\n")
		os.Exit(1)
	}
	fmt.Println("[PASS] Cryptographic signature VERIFIED.")

	// 5. Get Dev/Ino
	fileInfo, err := os.Stat(binaryPath)
	if err != nil {
		fmt.Printf("[FAIL] Failed to stat binary: %v\n", err)
		os.Exit(1)
	}
	stat, ok := fileInfo.Sys().(*syscall.Stat_t)
	if !ok {
		fmt.Printf("[FAIL] Failed to get underlying stat\n")
		os.Exit(1)
	}
	dev := uint64(stat.Dev)
	ino := uint64(stat.Ino)

	// 6. Push to BPF Map
	bpfMap, err := ebpf.LoadPinnedMap(MapPath, nil)
	if err != nil {
		fmt.Printf("[FAIL] Failed to load pinned pcc_auth_map: %v\n", err)
		fmt.Println("       Is telos_daemon running?")
		os.Exit(1)
	}
	defer bpfMap.Close()

	key := InodeKey{Dev: dev, Ino: ino}
	val := uint32(1)

	if err := bpfMap.Update(&key, &val, ebpf.UpdateAny); err != nil {
		fmt.Printf("[FAIL] Failed to update BPF map: %v\n", err)
		os.Exit(1)
	}

	fmt.Printf("[PASS] AOT Registration complete. Map updated for (dev: %d, ino: %d)\n", dev, ino)
}
