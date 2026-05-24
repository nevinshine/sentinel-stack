package main

import (
	"fmt"
	"net"
	"os"
	"os/signal"
	"syscall"
)

func main() {
	// First connection, which should succeed
	conn, err := net.Dial("tcp", "1.1.1.1:80")
	if err != nil {
		fmt.Printf("Initial dial failed: %v\n", err)
		os.Exit(1)
	}
	// Keep connection open
	defer conn.Close()

	// Wait for SIGUSR1
	sigs := make(chan os.Signal, 1)
	signal.Notify(sigs, syscall.SIGUSR1)
	
	// Signal to harness that connection is established
	fmt.Println("READY") 
	
	<-sigs

	// Attempt second connection after receiving SIGUSR1
	_, err2 := net.Dial("tcp", "1.1.1.1:80")
	if err2 != nil {
		// Check if it's permission denied (EPERM)
		if netErr, ok := err2.(*net.OpError); ok {
			if sysErr, ok := netErr.Err.(*os.SyscallError); ok {
				if sysErr.Err == syscall.EPERM {
					fmt.Println("BLOCKED_BY_EPERM")
					os.Exit(0) // Success case for the test!
				}
			}
		}
		fmt.Printf("Failed with unexpected error: %v\n", err2)
		os.Exit(1)
	}
	
	fmt.Println("Second connection succeeded (LSM failed to block)")
	os.Exit(2)
}
