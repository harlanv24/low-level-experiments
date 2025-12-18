# Question and Learning Objectives

**Question**  
Does `std::chrono::steady_clock` give consistent per-iteration timing for a million-iteration no-op loop on my WSL2 setup, or does scheduler/virtualization noise dominate?

**Learning Objectives**
- Understand the intrinsic cost of the timing primitive before trusting any latency number.
- Identify which noise sources (timer cost vs scheduler vs cache warmup) matter when the workload is tiny.
- Practice recording assumptions so later experiments (pinned vs unpinned, branch vs no-branch) stay comparable.

# Experimental Setup

## Environment Snapshot
Record this before each measurement session so you can explain differences later.

- **CPU**: AMD Ryzen 7 3700X (16 logical / 8 cores, SMT on). Obtained via `lscpu`.
- **Hypervisor**: Microsoft Hyper-V (WSL2). From `lscpu | grep Hypervisor` and `uname -a`.
- **Kernel**: `5.15.146.1-microsoft-standard-WSL2 #1 SMP Thu Jan 11 04:09:03 UTC 2024` (`cat /proc/version`).
- **Governor / frequency policy**: cpufreq interface not exposed inside this WSL instance; assume host Windows controls scaling. Note this assumption because frequency drift would skew measurements.
- **Compiler & flags**: `g++ -O2 -std=c++20 latency_lab.cpp -o latency_lab`.
- **Timer**: `std::chrono::steady_clock` (maps to `CLOCK_MONOTONIC` here).

_Commands to refresh these facts later:_
```
lscpu
uname -a
cat /proc/version
```
If cpufreq files become available, add `cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor`.

## Measurement Plan
1. **Timer characterization**: call `steady_clock::now()` in a tight loop (`measure_timer_overhead`) to estimate per-call cost. This tests the assumption that the stopwatch noise is negligible compared to the workload.
2. **Loop measurement**: run the million-iteration loop (`measure_loop`) and record elapsed wall-clock time.
3. **Stability study**: repeat step 2 enough times (start with 100 samples) to see distribution; compare mean vs percentiles. Tag each run with conditions (first run vs warmed cache, pinned/unpinned once pinning is added).
4. **Interpretation**: if loop time ≈ timer cost, enlarge workload or switch timer (e.g., `clock_gettime` or `rdtsc`). If variance is large despite timer being cheap, attribute it to scheduler/virtualization and plan pinning or isolation experiments.

## Assumptions to Track
- `steady_clock` is monotonic and has stable per-call cost (validated in step 1).
- `volatile` sink prevents the compiler from removing loop work.
- OS scheduling interruptions are rare; large outliers likely indicate descheduling.
- CPU frequency remains effectively constant during short runs (may be false if host throttles).

Whenever an assumption fails (e.g., timer cost spikes, cpufreq becomes available and shows scaling), record it here before tweaking code so future results remain reproducible.

## Run Log
| Run # | Timer cost (s) | Loop iterations | Loop time (s) | Overhead to Loop time ratio | Conditions / Notes |
|-------|----------------|-----------------|---------------|-----------------------------|--------------------|
| 1 | 2.21148e-08 | 1000000 | 0.000242466 | 9.12078e-05 | cold, unpinned |
| 2 | 2.28119e-08 | 1000000 | 0.000244951 | 9.31283e-05 | warm, unpinned |
| 3 | 2.78393e-08 | 1000000 | 0.000303664 | 9.1678e-05 | removed (void)t |
| 4 | 2.77243e-08 | 1000000 | 0.000303485 | 9.1353e-05 | removed volatile t |
| 5 | 2.29947e-08 | 1000000 | 3.2e-08 | 0.718586 | removed volatile sink |
| 6 | 2.25457e-08 | 1000000 | 0.000244143 | 9.23462e-05 | O3 compiler flag with both volatiles |
| 7 | 2.83204e-08 | 1000000 | 0.000303755 | 9.32343e-05 | O3 compiler flag with both volatiles and (void)t |
| 8 | 2.78057e-08 | 1000000 | 0.00030312 | 9.17317e-05 | pinned core 3, warm |
| 9 | 2.83013e-08 | 1000000 | 0.000302762 | 9.3477e-05 | pinned core 4, warm |
|10 | 2.80308e-08 | 1000000 | 0.000303001 | 9.25105e-05 | pinned core 3 via sched_setaffinity, warm |
|11 | 2.35817e-08 | 1000000 | 0.000495199 | 4.76207e-05 | pinned core 1 via sched_setaffinity, warm |
|12 | 2.35169e-08 | 1000000 | 0.000551104 | 4.26723e-05 | pinned core 1 via sched_setaffinity, warm |
|13 | 2.82302e-08 | 1000000 | 0.000606973 | 4.65098e-05 | pinned core 2 via sched_setaffinity, warm |
|14 | 2.83336e-08 | 1000000 | 0.000571813 | 4.95505e-05 | pinned core 3 via sched_setaffinity, warm |
|15 | 2.77128e-08 | 1000000 | 0.000606973 | 4.56573e-05 | pinned core 4 via sched_setaffinity, warm |
|16 | 2.76298e-08 | 1000000 | 0.00060729 | 4.54968e-05 | using clock_gettime instead of std::chrono |
|17 | 2.41068e-08 | 1000000 | 0.000251451 | 9.58707e-05 | using clock_gettime instead of std::chrono |
|18 | 2.47477e-08 | 1000000 | 0.000259373 | 9.54135e-05 | using clock_gettime instead of std::chrono |
|19 | 2.45958e-08 | 1000000 | 0.000253905 | 9.68699e-05 | pinned core 5 using clock_gettime instead of std::chrono |

## Analysis
- It looks like across the board, nothing really changed the ratio of overhead to loop time.
- The only outlier of course was removing the volatile on the sink, which effectively brought the loop time to be the same as the timer overhead.
- This indicates two things to me. First, the volatile actually does affect the compiler (obviously, but was helpful to see in practice.) Second, that the timer overhead is essentially nothing considering I removed the volatile t in the overhead loop and saw basically no change in time.
- Sometimes, I saw the overhead ratio almost half from 9e-05 to about 4.5e-05. Not really sure why this happened and why this number repeatedly, but it didn't seem very reproducible.
- No real advantage of calling clock_gettime over steady_clock.
- Didn't notice a difference changing the compiler optimization level, maybe the way my code was written didn't make a difference, or I used it wrong.

## Things I Learned
- It seems like steady_clock is already super low overhead and pretty fast. I think it's safe to use to measure latency.
- Learned how to CPU pin via code after reading Linux docs. Also learned how to log what CPU the code is running on at a particular time (`sched_getcpu()`).
- Learned how to interact with the system clock after reading Linux docs.
- Learned that `volatile` actually does prevent the compiler from optimizing code away.
- Even when I didn't pin the CPU, the scheduler happened to run the two functions on the same CPU anyways since the code was pretty quick. The Linux scheduler prefers cache locality, but the OS could still migrate the threads later.
- I don't think pinning did anything because the functions run serially anyways. It's not like they have a cache to reuse anyways.
- Pinning would make things worse if the host is busy, but otherwise pinning doesn't seem to change the latency.

## Helpful Resources
- [clock_gettime Linux docs](https://man7.org/linux/man-pages/man2/clock_gettime.2.html)
- [sched_setaffinity Linux docs](https://man7.org/linux/man-pages/man2/sched_setaffinity.2.html)
- [CPU set macros](https://man7.org/linux/man-pages/man3/CPU_SET.3.html)
