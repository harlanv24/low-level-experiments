| Run # | Timer cost (s) | Loop iterations | Loop time (s) | Overhead to Loop time ratio | Conditions / Notes |
|-------|----------------|-----------------|---------------|-----------------------------|--------------------|
| 1 | 2.21148e-08 | 1000000 | 0.000242466 | 9.12078e-05 | cold, unpinned |
| 1 | 2.28119e-08 | 1000000 | 0.000244951 | 9.31283e-05 | warm, unpinned |
| 1 | 2.78393e-08 | 1000000 | 0.000303664 | 9.1678e-05 | removed (void)t |
| 1 | 2.77243e-08 | 1000000 | 0.000303485 | 9.1353e-05 | removed volatile t |
| 1 | 2.29947e-08 | 1000000 | 3.2e-08 | 0.718586 | removed volatile sink |
| 1 | 2.25457e-08 | 1000000 | 0.000244143 | 9.23462e-05 | O3 compiler flag with both volatiles |
| 1 | 2.83204e-08 | 1000000 | 0.000303755 | 9.32343e-05 | O3 compiler flag with both volatiles and (void)t |
| 1 | 2.78057e-08 | 1000000 | 0.00030312 | 9.17317e-05 | pinned core 3, warm |
| 1 | 2.83013e-08 | 1000000 | 0.000302762 | 9.3477e-05 | pinned core 4, warm |
| 1 | 2.80308e-08 | 1000000 | 0.000303001 | 9.25105e-05 | pinned core 3 via sched_setaffinity, warm |
| 1 | 2.35817e-08 | 1000000 | 0.000495199 | 4.76207e-05 | pinned core 1 via sched_setaffinity, warm |
| 1 | 2.35169e-08 | 1000000 | 0.000551104 | 4.26723e-05 | pinned core 1 via sched_setaffinity, warm |
| 1 | 2.82302e-08 | 1000000 | 0.000606973 | 4.65098e-05 | pinned core 2 via sched_setaffinity, warm |
| 1 | 2.83336e-08 | 1000000 | 0.000571813 | 4.95505e-05 | pinned core 3 via sched_setaffinity, warm |
| 1 | 2.77128e-08 | 1000000 | 0.000606973 | 4.56573e-05 | pinned core 4 via sched_setaffinity, warm |
| 1 | 2.76298e-08 | 1000000 | 0.00060729 | 4.54968e-05 | using clock_gettime instead of std::chrono || 1 | 2.41068e-08 | 1000000 | 0.000251451 | 9.58707e-05 | using clock_gettime instead of std::chrono |
| 1 | 2.47477e-08 | 1000000 | 0.000259373 | 9.54135e-05 | using clock_gettime instead of std::chrono |
| 1 | 2.45958e-08 | 1000000 | 0.000253905 | 9.68699e-05 | pinned core 5 using clock_gettime instead of std::chrono |
| 1 | 3.03044e-08 | 1000000 | 0.000440614 | 6.87778e-05 | pinned core 5 using clock_gettime instead of std::chrono |
| 1 | 2.98093e-08 | 1000000 | 0.000304258 | 9.79738e-05 | pinned core 5 using clock_gettime instead of std::chrono |
