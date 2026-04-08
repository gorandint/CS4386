# Ataxx Heuristic Iteration Report

Run tag: 1775633620641

All heuristics in this run use intrinsic material coefficient = 50.
MaterialBoost is disabled in round 5 and round 6, and only enabled in round 7 for top models.

## Stage 5 - Unified Material Base

- CSV: out/stage5_unified_base_1775633620641.csv
- Format: iterative deepening only, time limit 1000 ms, 10 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Avg Move Time (ms) |
|---:|---|---:|---:|
| 1 | Hybrid | 50 | 24.31 |
| 2 | PositionWeight | 45 | 12.42 |
| 3 | Material | 45 | 12.45 |
| 4 | Expansion | 45 | 18.20 |
| 5 | PotentialConversion | 40 | 12.65 |
| 6 | Adaptive | 40 | 18.23 |
| 7 | Mobility | 40 | 66.31 |
| 8 | Aggression | 30 | 38.14 |
| 9 | ControlArea | 25 | 13.02 |

## Stage 6 - Filtered Candidates

- CSV: out/stage6_filtered_1775633620641.csv
- Format: iterative deepening only, time limit 1000 ms, 10 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Avg Move Time (ms) |
|---:|---|---:|---:|
| 1 | Hybrid | 25 | 20.40 |
| 2 | Material | 20 | 9.43 |
| 3 | PotentialConversion | 20 | 10.83 |
| 4 | Expansion | 20 | 14.51 |
| 5 | PositionWeight | 15 | 10.58 |

## Stage 7 - MaterialBoost On Top Models

- CSV: out/stage7_boost_tuning_1775633620641.csv
- Format: iterative deepening only, time limit 1000 ms, 10 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Avg Move Time (ms) |
|---:|---|---:|---:|
| 1 | Hybrid+Mat40 | 60 | 19.80 |
| 2 | Material | 35 | 11.22 |
| 3 | Material+Mat80 | 35 | 11.34 |
| 4 | Material+Mat40 | 35 | 11.35 |
| 5 | Material+Mat120 | 35 | 11.36 |
| 6 | Hybrid | 35 | 21.69 |
| 7 | Hybrid+Mat80 | 30 | 19.87 |
| 8 | Hybrid+Mat120 | 15 | 12.38 |

## Selection Summary

Best heuristic after round 7: **Hybrid+Mat40** (wins=60, avg move time=19.80 ms).

Round 5 input: Material + previously strong models + newly proposed models.

