# Sentinel Benchmark Suite

This directory contains the automated performance extraction and graphing suite for the Sentinel Stack.

## Objective
To deterministically measure and visualize the performance delta between Sentinel's **Ring 0 Data Plane (eBPF)** and its **User-Space Control Plane (Go Daemon)**.

## The Ring Buffer Saturation Insight
eBPF hooks intercept system calls and network packets at wire-speed directly inside the kernel. When an event is intercepted (e.g., dropping a socket `connect()`), it is pushed to an eBPF Ring Buffer. The user-space daemon asynchronously polls this buffer to write audit logs and increment Prometheus telemetry.

During a volumetric attack (e.g., 50,000 requests per second), the kernel effortlessly blocks 100% of the traffic. However, the 256KB Ring Buffer acts as a shock absorber. The user-space daemon simply cannot context-switch fast enough to read every event before the buffer rolls over. 

This results in a beautiful **Saturation Curve**, demonstrating that the security enforcement remains perfectly intact and bounded, while the observability telemetry gently flattens out, proving that the host CPU will not lock up under fire.

## Usage

1. **Install Dependencies:**
   ```bash
   pip install -r requirements.txt
   ```

2. **Extract Metrics:**
   This script runs the Red Team payload against the Vagrant Digital Twin (`intelhost`), stepping from 1,000 to 50,000 requests. It records the telemetry captured and the daemon's CPU overhead.
   ```bash
   python3 extract_metrics.py
   ```

3. **Generate Graphs:**
   Reads the CSV output and generates publication-ready SVGs using `matplotlib` and `seaborn`.
   ```bash
   python3 plot_performance.py
   ```

## Output Artifacts
- `results/saturation_curve.svg`: Plots the 1:1 kernel drop rate vs. the captured telemetry rate.
- `results/resource_overhead.svg`: Proves the daemon's CPU footprint remains isolated and bounded.
