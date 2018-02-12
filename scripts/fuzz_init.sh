sudo bash -c 'echo core >/proc/sys/kernel/core_pattern'
cd /sys/devices/system/cpu
sudo bash -c 'echo performance | tee cpu*/cpufreq/scaling_governor'
