import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import os

RESULTS_DIR = "tests/benchmarks/results"
TELEMETRY_CSV = f"{RESULTS_DIR}/telemetry_saturation.csv"
OVERHEAD_CSV = f"{RESULTS_DIR}/daemon_overhead.csv"

def plot_saturation_curve():
    if not os.path.exists(TELEMETRY_CSV):
        print(f"Error: {TELEMETRY_CSV} not found.")
        return

    df = pd.read_csv(TELEMETRY_CSV)
    
    sns.set_theme(style="whitegrid")
    plt.figure(figsize=(10, 6))
    
    # Plot Theoretical (1:1)
    plt.plot(df['target_flood'], df['target_flood'], linestyle='--', color='gray', label='Theoretical Interdictions (Kernel Drops)')
    
    # Plot Captured
    sns.lineplot(data=df, x='target_flood', y='captured_blocks', marker='o', color='red', label='Captured Telemetry (Userspace Daemon)')
    
    plt.title('Sentinel eBPF Ring Buffer Saturation Curve', fontsize=16, pad=15)
    plt.xlabel('Attack Volume (Total Connections Blocked)', fontsize=12)
    plt.ylabel('Events Successfully Processed by Daemon', fontsize=12)
    plt.legend(loc='upper left')
    
    # Annotate the drop-off
    max_cap = df['captured_blocks'].max()
    plt.axhline(max_cap, color='red', linestyle=':', alpha=0.5)
    plt.text(df['target_flood'].max(), max_cap - 2000, f'Saturation Point\n(~{max_cap} events)', color='red', ha='right')
    
    plt.tight_layout()
    plt.savefig(f"{RESULTS_DIR}/saturation_curve.svg")
    print(f"Saved {RESULTS_DIR}/saturation_curve.svg")

def plot_resource_overhead():
    if not os.path.exists(OVERHEAD_CSV):
        print(f"Error: {OVERHEAD_CSV} not found.")
        return

    df = pd.read_csv(OVERHEAD_CSV)
    
    sns.set_theme(style="whitegrid")
    fig, ax1 = plt.subplots(figsize=(10, 6))

    # CPU Plot
    ax1.set_xlabel('Attack Volume (Connections)', fontsize=12)
    ax1.set_ylabel('CPU Utilization (%)', color='blue', fontsize=12)
    sns.lineplot(data=df, x='target_flood', y='usr_cpu', marker='s', color='blue', label='User CPU', ax=ax1)
    sns.lineplot(data=df, x='target_flood', y='sys_cpu', marker='^', color='cyan', label='System CPU', ax=ax1)
    ax1.tick_params(axis='y', labelcolor='blue')
    ax1.set_ylim(0, 100)
    
    # RSS Plot
    ax2 = ax1.twinx()
    ax2.set_ylabel('Resident Set Size (MB)', color='green', fontsize=12)
    # Convert KB to MB
    df['rss_mb'] = df['rss_kb'] / 1024
    sns.lineplot(data=df, x='target_flood', y='rss_mb', marker='o', color='green', label='RSS Memory', ax=ax2)
    ax2.tick_params(axis='y', labelcolor='green')
    ax2.set_ylim(0, df['rss_mb'].max() * 1.5 if not df.empty else 100)
    
    plt.title('Telos Daemon Resource Overhead Under Flood', fontsize=16, pad=15)
    
    # Combine legends
    lines_1, labels_1 = ax1.get_legend_handles_labels()
    lines_2, labels_2 = ax2.get_legend_handles_labels()
    ax1.legend(lines_1 + lines_2, labels_1 + labels_2, loc='upper left')
    
    plt.tight_layout()
    plt.savefig(f"{RESULTS_DIR}/resource_overhead.svg")
    print(f"Saved {RESULTS_DIR}/resource_overhead.svg")

if __name__ == "__main__":
    plot_saturation_curve()
    plot_resource_overhead()
