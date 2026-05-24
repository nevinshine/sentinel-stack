package main

import (
	"bufio"
	"context"
	"encoding/json"
	"log"
	"os"
	"sync"
	"time"

	pb "github.com/nevinshine/sentinel-vmi/sidecar/pb/vmi"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
)

func main() {
	target := os.Getenv("SIEM_GRPC_TARGET")
	if target == "" {
		target = "127.0.0.1:50052"
	}

	log.Printf("Starting Sentinel VMI gRPC Sidecar (Target: %s)", target)

	conn, err := grpc.NewClient(target, grpc.WithTransportCredentials(insecure.NewCredentials()))
	if err != nil {
		log.Fatalf("Failed to connect to SIEM: %v", err)
	}
	defer conn.Close()

	client := pb.NewAlertForwarderClient(conn)

	// Create a buffered channel to absorb bursts (e.g., 1000 alerts)
	alertQueue := make(chan pb.Alert, 1000)
	var wg sync.WaitGroup
	wg.Add(1)

	// Worker Goroutine: Handles the gRPC network calls
	go func() {
		defer wg.Done()
		for alert := range alertQueue {
			ctx, cancel := context.WithTimeout(context.Background(), 2*time.Second)
			_, err := client.PushAlert(ctx, &alert)
			cancel()
			if err != nil {
				log.Printf("ERROR: Failed to push alert to SIEM (PID %d): %v", alert.Pid, err)
			}
		}
	}()

	// Main Goroutine: Strictly reads stdin as fast as possible
	scanner := bufio.NewScanner(os.Stdin)
	for scanner.Scan() {
		var alert pb.Alert
		if err := json.Unmarshal(scanner.Bytes(), &alert); err == nil {
			// Non-blocking send: if the queue is full, drop the alert to save the hypervisor
			select {
			case alertQueue <- alert:
				// Successfully queued
			default:
				log.Println("WARN: SIEM backpressure. Dropping alert to prevent hypervisor lockup.")
			}
		} else {
			log.Printf("ERROR: Failed to parse alert JSON: %v", err)
		}
	}

	if err := scanner.Err(); err != nil {
		log.Printf("ERROR: stdin scanner failed: %v", err)
	}
	
	close(alertQueue)
	wg.Wait()
	log.Println("Sentinel VMI Sidecar exiting gracefully.")
}
