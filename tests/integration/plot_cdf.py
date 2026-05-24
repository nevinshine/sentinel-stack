import json
import numpy as np
import matplotlib.pyplot as plt

# Load the raw benchmark data
with open('latencies.json', 'r') as f:
    data = json.load(f)

# Sort the data to compute cumulative probabilities
sorted_data = np.sort(data)
p = 1. * np.arange(len(sorted_data)) / (len(sorted_data) - 1)

# Configure publication-style plot settings
plt.figure(figsize=(8, 5))
plt.style.use('seaborn-v0_8-whitegrid')

# Plot the CDF
plt.plot(sorted_data, p, linewidth=2, color='#004488')

# Annotate key percentiles
percentiles = [50, 95, 99]
for pct in percentiles:
    val = np.percentile(sorted_data, pct)
    plt.axvline(val, color='red', linestyle='--', alpha=0.5)
    plt.text(val + 0.05, 0.1, f'p{pct}\n{val:.2f}ms', color='red', fontsize=10)

# Labels and formatting
plt.title('CDF of NetworkSlam Enforcement Latency (1000 Iterations)', fontsize=14, fontweight='bold')
plt.xlabel('Enforcement Latency (ms)', fontsize=12)
plt.ylabel('Cumulative Probability', fontsize=12)
plt.xlim(left=0, right=max(sorted_data) + 0.5)
plt.ylim(0, 1.05)

# Save as a vector graphic for LaTeX inclusion
plt.tight_layout()
plt.savefig('network_slam_cdf.pdf', format='pdf', dpi=300)
print("Saved CDF plot to network_slam_cdf.pdf")
