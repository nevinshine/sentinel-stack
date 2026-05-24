/* HYPERION CONTROLLER M5 (Telemetry + Flow Tracking) */
package main

import (
	"bufio"
	"bytes"
	"encoding/binary"
	"flag"
	"fmt"
	"log"
	"net"
	"os"
	"os/signal"
	"strings"
	"syscall"
	"time"

	"github.com/cilium/ebpf"
	"github.com/cilium/ebpf/link"
	"github.com/cilium/ebpf/ringbuf"
	"github.com/cilium/ebpf/rlimit"
)

// --- COLORS & VISUALS ---
const (
	ColorReset  = "\033[0m"
	ColorRed    = "\033[31m"
	ColorGreen  = "\033[32m"
	ColorYellow = "\033[33m"
	ColorBlue   = "\033[34m"
	ColorPurple = "\033[35m"
	ColorCyan   = "\033[36m"
	ColorWhite  = "\033[37m"
)

const (
	ConfigFile = "signatures.txt"
	MaxRules   = 2
)

// Must match Kernel Struct (struct policy_t)
type Policy struct {
	Signature [8]uint8
	SigLen    uint8
	Active    uint8
	Pad       [2]uint8 // FIXED: Aligns to 12 bytes
}

// M5: New telemetry event structure (must match struct hyp_event)
type HypEvent struct {
	EventType uint8    // 0=ACCEPT, 1=DROP, 2=SIG_MATCH
	_         [3]uint8 // Padding for alignment
	SrcIP     uint32
	DstIP     uint32
	SrcPort   uint16
	DstPort   uint16
	Protocol  uint8
	_         [7]uint8 // Padding for 8-byte alignment before Timestamp
	Timestamp uint64
	Signature [8]byte
}

// Must match Kernel Struct (struct event_t)
type AlertEvent struct {
	SrcIP   uint32
	DstIP   uint32
	SrcPort uint16 // FIXED: Added to match C
	DstPort uint16 // FIXED: Added to match C
	Action  uint32 // FIXED: Renamed from RuleID (since C sends Action)
	Snippet [8]byte
}

var loadedSigs []string
var bootTimeOffset int64 // Boot time offset to convert bpf_ktime_get_ns() to Unix time

//go:generate go run github.com/cilium/ebpf/cmd/bpf2go -cc $BPF_CLANG -cflags $BPF_CFLAGS bpf ../kern/hyperion_core.c -- -I../common

func main() {
	ifaceName := flag.String("iface", "wlp1s0", "Interface to attach XDP")
	sigFlag := flag.String("sig", "", "Comma-separated signatures (e.g., -sig \"hack,malware,evil\")")
	telemetryFlag := flag.Bool("telemetry", false, "Enable telemetry event output")
	logfileFlag := flag.String("logfile", "", "Optional file for logging events")
	blockIpsFlag := flag.String("block-ips", "", "Comma-separated CIDRs to block")
	flag.Parse()

	printBanner()

	// 0. Calculate boot time offset for timestamp conversion
	// bpf_ktime_get_ns() returns nanoseconds since boot, not Unix epoch
	if err := calculateBootTimeOffset(); err != nil {
		log.Fatalf("%s[!] Failed to calculate boot time offset: %v%s", ColorRed, err, ColorReset)
	}

	// 1. Init
	if err := rlimit.RemoveMemlock(); err != nil {
		log.Fatal(err)
	}

	pinnedPath := "/sys/fs/bpf/telos/sentinel_events"
	var mapReplacements map[string]*ebpf.Map

	if _, err := os.Stat(pinnedPath); err == nil {
		sharedMap, err := ebpf.LoadPinnedMap(pinnedPath, nil)
		if err == nil {
			log.Printf("%s[+] Found pinned Ring Buffer. Redirecting fast-path telemetry...%s", ColorGreen, ColorReset)
			mapReplacements = map[string]*ebpf.Map{
				"sentinel_events": sharedMap,
			}
		} else {
			log.Printf("%s[!] Failed to load pinned sentinel_events map: %v%s", ColorYellow, err, ColorReset)
		}
	} else {
		log.Printf("%s[i] Pinned Ring Buffer path %s not found (normal if telos-daemon not running yet)%s", ColorYellow, pinnedPath, ColorReset)
	}

	// Check for pinned vmi_alert_map
	vmiAlertPinnedPath := "/sys/fs/bpf/telos/vmi_alert_map"
	if _, err := os.Stat(vmiAlertPinnedPath); err == nil {
		if sharedVmiMap, err := ebpf.LoadPinnedMap(vmiAlertPinnedPath, nil); err == nil {
			log.Printf("%s[+] Found pinned vmi_alert_map. Redirecting alert interface...%s", ColorGreen, ColorReset)
			if mapReplacements == nil {
				mapReplacements = make(map[string]*ebpf.Map)
			}
			mapReplacements["vmi_alert_map"] = sharedVmiMap
		} else {
			log.Printf("%s[!] Failed to load pinned vmi_alert_map: %v%s", ColorYellow, err, ColorReset)
		}
	}

	var opts ebpf.CollectionOptions
	if mapReplacements != nil {
		opts.MapReplacements = mapReplacements
	}

	objs := bpfObjects{}
	if err := loadBpfObjects(&objs, &opts); err != nil {
		log.Fatalf("%s[!] Load BPF failed: %v%s", ColorRed, err, ColorReset)
	}
	defer objs.Close()

	// Pin hyperion_blocklist map so telos_daemon can add entries for NetworkSlam
	blocklistPinnedPath := "/sys/fs/bpf/telos/hyperion_blocklist"
	os.Remove(blocklistPinnedPath) // Ensure we overwrite any stale pin
	if err := objs.BlocklistMap.Pin(blocklistPinnedPath); err != nil {
		log.Printf("%s[!] Warning: Failed to pin hyperion_blocklist: %v%s", ColorYellow, err, ColorReset)
	} else {
		log.Printf("%s[+] Pinned hyperion_blocklist to %s%s", ColorGreen, blocklistPinnedPath, ColorReset)
	}

	// M4: Drawbridge IP Blocklist
	if *blockIpsFlag != "" {
		var keys []LPMKey
		var values []uint8
		for _, cidr := range strings.Split(*blockIpsFlag, ",") {
			cidr = strings.TrimSpace(cidr)
			if cidr == "" {
				continue
			}
			if !strings.Contains(cidr, "/") {
				cidr += "/32"
			}
			_, ipNet, err := net.ParseCIDR(cidr)
			if err != nil {
				log.Fatalf("%s[!] Invalid CIDR %s: %v%s", ColorRed, cidr, err, ColorReset)
			}
			prefixLen, _ := ipNet.Mask.Size()

			// To match kernel uint32 which expects raw network bytes
			ipUint32 := binary.LittleEndian.Uint32(ipNet.IP.To4())

			keys = append(keys, LPMKey{PrefixLen: uint32(prefixLen), IP: ipUint32})
			values = append(values, 1) // 1 = blocked
		}

		hekiSocket := os.Getenv("TELOS_HEKI_VMI_SOCKET")
		if hekiSocket == "" {
			hekiSocket = "/tmp/heki.sock"
		}
		nonce, err := RegisterMap(hekiSocket, "blocklist_map", 0, 4096, true)
		if err != nil {
			log.Fatalf("%s[!] HEKI Registration failed: %v%s", ColorRed, err, ColorReset)
		}

		err = ExecuteDrawbridgeUpdate(nonce, keys, values, objs.BlocklistMap)
		if err != nil {
			log.Fatalf("%s[!] Drawbridge BatchUpdate failed: %v%s", ColorRed, err, ColorReset)
		}
		fmt.Printf("%s[+] Drawbridge IP Blocklist loaded: %d subnets%s\n", ColorGreen, len(keys), ColorReset)
	}

	// 2. Load Config
	if err := reloadSignatures(objs.PolicyMap, *sigFlag); err != nil {
		log.Printf("%s[!] Initial load warning: %v%s", ColorYellow, err, ColorReset)
	}

	// 3. Attach XDP
	iface, err := net.InterfaceByName(*ifaceName)
	if err != nil {
		log.Fatalf("%s[!] Interface %s not found%s", ColorRed, *ifaceName, ColorReset)
	}

	l, err := link.AttachXDP(link.XDPOptions{
		Program:   objs.HyperionFilter,
		Interface: iface.Index,
	})
	if err != nil {
		log.Fatalf("%s[!] XDP Attach failed: %v%s", ColorRed, err, ColorReset)
	}
	defer l.Close()

	// 4. Setup optional log file
	var logFile *os.File
	if *logfileFlag != "" {
		logFile, err = os.OpenFile(*logfileFlag, os.O_CREATE|os.O_WRONLY|os.O_APPEND, 0644)
		if err != nil {
			log.Fatalf("%s[!] Failed to open log file: %v%s", ColorRed, err, ColorReset)
		}
		defer logFile.Close()
		fmt.Printf("%s[+] Logging events to: %s%s\n", ColorGreen, *logfileFlag, ColorReset)
	}

	// 5. Legacy Telemetry Loop (alert_ringbuf)
	rd, err := ringbuf.NewReader(objs.AlertRingbuf)
	if err != nil {
		log.Fatalf("%s[!] Ringbuf failed: %v%s", ColorRed, err, ColorReset)
	}
	defer rd.Close()

	// 6. M5: New Telemetry Loop (telemetry_ringbuf)
	var telemetryReader *ringbuf.Reader
	if *telemetryFlag {
		telemetryReader, err = ringbuf.NewReader(objs.TelemetryRingbuf)
		if err != nil {
			log.Fatalf("%s[!] Telemetry Ringbuf failed: %v%s", ColorRed, err, ColorReset)
		}
		defer telemetryReader.Close()
		fmt.Printf("%s[+] Telemetry enabled%s\n", ColorGreen, ColorReset)
	}

	fmt.Printf("%s[+] Hyperion Active on %s%s\n", ColorGreen, *ifaceName, ColorReset)
	fmt.Printf("%s[i] PID: %d (Run 'kill -HUP %d' to reload)%s\n", ColorCyan, os.Getpid(), os.Getpid(), ColorReset)
	fmt.Println(strings.Repeat("-", 60))
	fmt.Printf("%sWaiting for threats...%s\n", ColorWhite, ColorReset)

	// Legacy alert handler
	go func() {
		for {
			record, err := rd.Read()
			if err != nil {
				if err == ringbuf.ErrClosed {
					return
				}
				continue
			}

			var event AlertEvent
			// Binary read must match struct layout exactly
			if err := binary.Read(bytes.NewReader(record.RawSample), binary.LittleEndian, &event); err != nil {
				log.Printf("Failed to parse event: %v", err)
				continue
			}

			// NOTE: Currently C sends Action=1 (Drop), not the Rule Index.
			// So we just log "BLOCKED" generally.
			payloadStr := string(event.Snippet[:])
			// sanitize string
			payloadStr = strings.ReplaceAll(payloadStr, "\n", ".")
			payloadStr = strings.ReplaceAll(payloadStr, "\r", ".")

			// ALERT LOG STYLE
			fmt.Printf("%s[%s] ALERT: Blocked Traffic from %s -> Payload: [%s]%s\n",
				ColorRed,
				time.Now().Format("15:04:05"),
				int2ip(event.SrcIP),
				payloadStr,
				ColorReset)
		}
	}()

	// M5: Telemetry event handler
	if *telemetryFlag {
		go func() {
			for {
				record, err := telemetryReader.Read()
				if err != nil {
					if err == ringbuf.ErrClosed {
						return
					}
					continue
				}

				var event HypEvent
				if err := binary.Read(bytes.NewReader(record.RawSample), binary.LittleEndian, &event); err != nil {
					log.Printf("Failed to parse telemetry event: %v", err)
					continue
				}

				// Format the event
				eventStr := formatTelemetryEvent(&event)

				// Print to stdout
				fmt.Println(eventStr)

				// Optionally write to log file
				if logFile != nil {
					fmt.Fprintln(logFile, eventStr)
				}
			}
		}()
	}

	// 7. Signal Handling
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM, syscall.SIGHUP)

	for {
		sig := <-sigChan
		switch sig {
		case syscall.SIGHUP:
			fmt.Printf("\n%s[!] Reloading signatures...%s\n", ColorYellow, ColorReset)
			if err := reloadSignatures(objs.PolicyMap, *sigFlag); err != nil {
				fmt.Printf("%s[-] Reload Error: %v%s\n", ColorRed, err, ColorReset)
			} else {
				fmt.Printf("%s[+] Reload Complete.%s\n", ColorGreen, ColorReset)
			}
		case syscall.SIGINT, syscall.SIGTERM:
			fmt.Printf("\n%s[-] Shutting down Hyperion.%s\n", ColorRed, ColorReset)
			return
		}
	}
}

func reloadSignatures(m *ebpf.Map, cliSig string) error {
	var newSigs []string

	// If CLI flag is provided, use it; otherwise fall back to file
	if cliSig != "" {
		// Parse comma-separated signatures from CLI
		for _, sig := range strings.Split(cliSig, ",") {
			sig = strings.TrimSpace(sig)
			if sig != "" {
				newSigs = append(newSigs, sig)
			}
		}
	} else {
		// Fall back to file-based config
		f, err := os.Open(ConfigFile)
		if err != nil {
			return err
		}
		defer f.Close()

		sc := bufio.NewScanner(f)
		for sc.Scan() {
			line := strings.TrimSpace(sc.Text())
			if line != "" && !strings.HasPrefix(line, "#") {
				newSigs = append(newSigs, line)
			}
		}
	}

	if len(newSigs) > MaxRules {
		return fmt.Errorf("Too many rules (Max %d)", MaxRules)
	}

	for i := 0; i < MaxRules; i++ {
		var pol Policy
		if i < len(newSigs) {
			s := newSigs[i]
			if len(s) > 8 {
				s = s[:8]
			}
			copy(pol.Signature[:], []byte(s))
			pol.SigLen = uint8(len(s)) // FIXED: Type cast
			pol.Active = 1
			fmt.Printf("    %s-> Loaded Rule %d: %s%s\n", ColorBlue, i, s, ColorReset)
		} else {
			pol.Active = 0
		}

		// FIXED: Use uint32(i) for key to match BPF definition
		if err := m.Put(uint32(i), pol); err != nil {
			return err
		}
	}
	loadedSigs = newSigs
	return nil
}

func int2ip(nn uint32) net.IP {
	ip := make(net.IP, 4)
	binary.LittleEndian.PutUint32(ip, nn)
	return ip
}

// Calculate boot time offset to convert bpf_ktime_get_ns() to Unix time
func calculateBootTimeOffset() error {
	// Read system uptime from /proc/uptime
	data, err := os.ReadFile("/proc/uptime")
	if err != nil {
		return fmt.Errorf("failed to read /proc/uptime: %w", err)
	}

	// /proc/uptime contains two numbers: uptime in seconds and idle time
	// We only need the first number
	var uptimeSeconds float64
	_, err = fmt.Sscanf(string(data), "%f", &uptimeSeconds)
	if err != nil {
		return fmt.Errorf("failed to parse /proc/uptime: %w", err)
	}

	// Convert uptime to nanoseconds
	bootTimeNs := int64(uptimeSeconds * 1e9)

	// Calculate offset: current Unix time - boot time
	bootTimeOffset = time.Now().UnixNano() - bootTimeNs

	return nil
}

// M5: Format telemetry event for display
func formatTelemetryEvent(event *HypEvent) string {
	// Convert timestamp from boot time nanoseconds to Unix time
	// bpf_ktime_get_ns() returns nanoseconds since boot, so we add our offset
	t := time.Unix(0, int64(event.Timestamp)+bootTimeOffset)
	timestamp := t.Format("2006-01-02 15:04:05")

	// Convert IPs
	srcIP := int2ip(event.SrcIP)
	dstIP := int2ip(event.DstIP)

	// Convert ports from network byte order (big endian) to host byte order
	// Ports are stored in network byte order, so we just need to swap bytes
	srcPort := (event.SrcPort >> 8) | (event.SrcPort << 8)
	dstPort := (event.DstPort >> 8) | (event.DstPort << 8)

	// Determine protocol name
	protoName := "UNKNOWN"
	switch event.Protocol {
	case 6:
		protoName = "TCP"
	case 17:
		protoName = "UDP"
	}

	// Determine event type
	eventType := ""
	eventColor := ColorWhite
	switch event.EventType {
	case 0:
		eventType = "ACCEPT"
		eventColor = ColorGreen
	case 1:
		eventType = "DROP"
		eventColor = ColorRed
	case 2:
		eventType = "SIG_MATCH"
		eventColor = ColorYellow
	}

	// Format signature if present
	sigStr := ""
	if event.EventType == 1 || event.EventType == 2 {
		// Extract signature (null-terminated or fixed 8 bytes)
		sig := bytes.TrimRight(event.Signature[:], "\x00")
		if len(sig) > 0 {
			sigStr = fmt.Sprintf(" sig=\"%s\"", string(sig))
		}
	}

	return fmt.Sprintf("[%s] %s%s%s %s:%d -> %s:%d %s%s",
		timestamp,
		eventColor,
		eventType,
		ColorReset,
		srcIP,
		srcPort,
		dstIP,
		dstPort,
		protoName,
		sigStr)
}

func printBanner() {
	banner := `
%s    __  __                      _
   / / / /_  ______  ___  _____(_)___  ____
  / /_/ / / / / __ \/ _ \/ ___/ / __ \/ __ \
 / __  / /_/ / /_/ /  __/ /  / / /_/ / / / /
/_/ /_/\__, / .___/\___/_/  /_/\____/_/ /_/
      /____/_/

    %s:: Hyperion XDP Engine vM5 (Telemetry) ::%s
`
	fmt.Printf(banner, ColorCyan, ColorPurple, ColorReset)
	fmt.Println()
}
