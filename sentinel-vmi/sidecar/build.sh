#!/bin/bash
set -e
export PATH=$PATH:/home/nevin/go/bin

echo "Generating protobuf bindings..."
mkdir -p pb/vmi
protoc --go_out=./pb/vmi --go_opt=paths=source_relative --go-grpc_out=./pb/vmi --go-grpc_opt=paths=source_relative -I../proto ../proto/vmi_stream.proto

echo "Initializing Go module..."
go mod tidy

echo "Building sidecar forwarder..."
go build -o sidecar main.go

echo "Building dummy SIEM receiver..."
go build -o dummy_siem dummy_siem.go

echo "Build complete."
