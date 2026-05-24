package main

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"net"
)

type HekiDrawbridgeRequest struct {
	PayloadGva      uint64
	PayloadSize     uint32
	TargetSubsystem uint32
	EphemeralNonce  uint64
	Magic           uint32
}

func RegisterMap(socketPath string, mapName string, gva uint64, size uint32, isCritical bool) (uint64, error) {
	conn, err := net.Dial("unix", socketPath)
	if err != nil {
		return 0, fmt.Errorf("failed to dial HEKI socket: %w", err)
	}
	defer conn.Close()

	reg := HekiDrawbridgeRequest{
		PayloadGva:      gva,
		PayloadSize:     size,
		TargetSubsystem: 2, // HEKI_SUBSYSTEM_HYPERION
		EphemeralNonce:  0,
		Magic:           0x48454B49,
	}

	buf := new(bytes.Buffer)
	if err := binary.Write(buf, binary.LittleEndian, &reg); err != nil {
		return 0, fmt.Errorf("failed to pack struct: %w", err)
	}

	if _, err := conn.Write(buf.Bytes()); err != nil {
		return 0, fmt.Errorf("failed to write to socket: %w", err)
	}

	var nonce uint64
	if err := binary.Read(conn, binary.LittleEndian, &nonce); err != nil {
		return 0, fmt.Errorf("failed to read nonce from socket: %w", err)
	}

	return nonce, nil
}
