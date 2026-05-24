package main

import (
	"context"
	"log"
	"net"

	pb "github.com/nevinshine/sentinel-vmi/sidecar/pb/vmi"
	"google.golang.org/grpc"
)

type siemServer struct {
	pb.UnimplementedAlertForwarderServer
}

func (s *siemServer) PushAlert(ctx context.Context, in *pb.Alert) (*pb.Ack, error) {
	log.Printf("\033[1;31m[SIEM] ALERT RECEIVED! PID: %d, Threat Type: %s, Confidence: %.2f, Level: %d\033[0m", 
		in.Pid, in.ThreatType, in.Confidence, in.ThreatLevel)
	if in.Reason != "" {
		log.Printf("\033[1;33m[SIEM] Reason: %s\033[0m", in.Reason)
	}
	return &pb.Ack{Success: true}, nil
}

func main() {
	lis, err := net.Listen("tcp", "127.0.0.1:50052")
	if err != nil {
		log.Fatalf("failed to listen: %v", err)
	}
	
	s := grpc.NewServer()
	pb.RegisterAlertForwarderServer(s, &siemServer{})
	
	log.Printf("Dummy SIEM receiver listening at %v", lis.Addr())
	if err := s.Serve(lis); err != nil {
		log.Fatalf("failed to serve: %v", err)
	}
}
