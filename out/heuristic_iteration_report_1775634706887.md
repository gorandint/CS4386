# Ataxx Heuristic Iteration Report

Run tag: 1775634706887

All heuristics in this run use intrinsic material coefficient = 50.
MaterialBoost is disabled in round 5 and round 6, and only enabled in round 7 for top models.

## Stage 5 - Unified Material Base

- CSV: out/stage5_unified_base_1775634706887.csv
- Format: iterative deepening only, time limit 1000 ms, 10 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Avg Move Time (ms) |
|---:|---|---:|---:|
| 1 | Hybrid | 50 | 19.09 |
| 2 | Material | 45 | 9.80 |
| 3 | PositionWeight | 45 | 11.20 |
| 4 | Expansion | 45 | 14.26 |
| 5 | PotentialConversion | 40 | 11.38 |
| 6 | Adaptive | 40 | 16.77 |
| 7 | Mobility | 40 | 59.49 |
| 8 | Aggression | 30 | 35.12 |
| 9 | ControlArea | 25 | 12.38 |

## Stage 6 - Filtered Candidates

- CSV: out/stage6_filtered_1775634706887.csv
- Format: iterative deepening only, time limit 1000 ms, 10 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Avg Move Time (ms) |
|---:|---|---:|---:|
| 1 | Hybrid | 25 | 20.33 |
| 2 | Material | 20 | 9.40 |
| 3 | PotentialConversion | 20 | 10.77 |
| 4 | Expansion | 20 | 14.46 |
| 5 | PositionWeight | 15 | 10.57 |

## Stage 7 - MaterialBoost On Top Models

- CSV: out/stage7_boost_tuning_1775634706887.csv
- Format: iterative deepening only, time limit 1000 ms, 10 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Avg Move Time (ms) |
|---:|---|---:|---:|
| 1 | Hybrid+Mat40 | 60 | 19.80 |
| 2 | Material | 35 | 11.21 |
| 3 | Material+Mat120 | 35 | 11.30 |
| 4 | Material+Mat40 | 35 | 11.31 |
| 5 | Material+Mat80 | 35 | 11.34 |
| 6 | Hybrid | 35 | 21.60 |
| 7 | Hybrid+Mat80 | 30 | 19.85 |
| 8 | Hybrid+Mat120 | 15 | 12.42 |

## Stage 8 - Hybrid MaterialBoost Refinement

- CSV: out/stage8_hybrid_refine_1775634706887.csv
- Format: iterative deepening only, time limit 1000 ms, 10 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Avg Move Time (ms) |
|---:|---|---:|---:|
| 1 | Hybrid+Mat50 | 60 | 19.35 |
| 2 | Hybrid+Mat40 | 60 | 19.47 |
| 3 | Hybrid+Mat60 | 50 | 18.18 |
| 4 | Hybrid+Mat20 | 30 | 18.87 |
| 5 | Hybrid | 30 | 21.58 |
| 6 | Material | 20 | 10.01 |
| 7 | PotentialConversion | 20 | 11.41 |
| 8 | Hybrid+Mat30 | 10 | 15.63 |

## Selection Summary

Best heuristic after round 8: **Hybrid+Mat50** (wins=60, avg move time=19.35 ms).

Round 5 input: Material + previously strong models + newly proposed models.

