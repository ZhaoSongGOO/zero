#!/usr/bin/env python3
import re
import sys
import matplotlib.pyplot as plt

# 解析命令行参数
if len(sys.argv) < 2:
    print("Usage: python3 plot_memory.py <log_file> [sample_interval]")
    print("  log_file: Path to the log file")
    print("  sample_interval: Sample interval (default: 1, use -1 for all data)")
    sys.exit(1)

log_file = sys.argv[1]
sample_interval = int(sys.argv[2]) if len(sys.argv) > 2 else 1

# 读取日志文件
bytes_values = []

with open(log_file, "r") as f:
    for line in f:
        match = re.search(r"Memory\s+(\d+)\s+Byte", line)
        if match:
            bytes_values.append(int(match.group(1)))

# 取样
if sample_interval > 1:
    sampled_values = bytes_values[::sample_interval]
    print(f"Sampling interval: {sample_interval}")
elif sample_interval == -1:
    sampled_values = bytes_values
    print("Using all data points")
else:
    sampled_values = bytes_values

# 绘制曲线图
plt.figure(figsize=(14, 7))
plt.plot(
    range(len(sampled_values)),
    sampled_values,
    marker=".",
    linestyle="-",
    linewidth=1,
    markersize=3,
)
plt.xlabel("Time (samples)")
plt.ylabel("Memory (Bytes)")
plt.title(f"Memory Usage Over Time - {log_file}")
plt.grid(True, alpha=0.3)
plt.tight_layout()

# 显示统计信息
print(f"\nStatistics:")
print(f"  Total samples: {len(bytes_values)}")
print(f"  Displayed samples: {len(sampled_values)}")
print(f"  Min: {min(sampled_values)} Bytes")
print(f"  Max: {max(sampled_values)} Bytes")
print(f"  Final: {sampled_values[-1]} Bytes")

plt.show()
