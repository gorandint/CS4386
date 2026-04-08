# Ataxx Heuristic Iteration Report

Run tag: 1775638626415

All base heuristics in this run use intrinsic material coefficient = 50.
Ranking key: wins desc, timeout wins asc, avg move time asc.

## Stage 9 - Exp40 vs PC40 vs Hybrid40 vs Material

- CSV: out/stage9_core_compare_1775638626415.csv
- Format: iterative deepening only, time limit 1000 ms, 20 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Timeout Wins | Avg Move Time (ms) |
|---:|---|---:|---:|---:|
| 1 | Expansion+Mat40 | 40 | 0 | 24.38 |
| 2 | Hybrid+Mat40 | 40 | 20 | 28.93 |
| 3 | Material | 20 | 10 | 13.64 |
| 4 | PotentialConversion+Mat40 | 20 | 10 | 16.74 |

## Stage 10 - Recombine Under Mat40

- CSV: out/stage10_recombine_mat40_1775638626415.csv
- Format: iterative deepening only, time limit 1000 ms, 20 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Timeout Wins | Avg Move Time (ms) |
|---:|---|---:|---:|---:|
| 1 | Expansion+Mat40 | 80 | 10 | 28.98 |
| 2 | PressureExpansion+Mat40 | 80 | 30 | 30.81 |
| 3 | Hybrid+Mat40 | 70 | 20 | 27.38 |
| 4 | Material | 60 | 30 | 12.60 |
| 5 | PotentialConversion+Mat40 | 60 | 30 | 14.24 |
| 6 | CenterPressurePC+Mat40 | 50 | 50 | 22.00 |
| 7 | CenterExpansion+Mat40 | 20 | 0 | 29.95 |

## Stage 11 - AB Material vs ID Material

- CSV: out/stage11_ab_vs_id_material_1775638626415.csv
- Format: iterative deepening only, time limit 1000 ms, 10 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Timeout Wins | Avg Move Time (ms) |
|---:|---|---:|---:|---:|
| 1 | AB7-Material | 25 | 0 | 50.95 |
| 2 | AB6-Material | 20 | 0 | 14.22 |
| 3 | ID-Material | 15 | 10 | 13.25 |
| 4 | AB5-Material | 0 | 0 | 3.08 |

## Stage 12 - ID Hybrid vs MCTS Variants

- CSV: out/stage12_id_vs_mcts_1775638626415.csv
- Format: iterative deepening only, time limit 1000 ms, 10 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Timeout Wins | Avg Move Time (ms) |
|---:|---|---:|---:|---:|
| 1 | ID-Hybrid | 30 | 0 | 42.06 |
| 2 | MCTS-Material | 15 | 0 | 945.05 |
| 3 | MCTS-Hybrid | 11 | 0 | 950.19 |
| 4 | MCTS-Random | 4 | 0 | 979.98 |

## Selection Summary

Best heuristic after recombination: **Expansion+Mat40** (wins=80, timeout wins=10, avg move time=28.98 ms).

Round 9 compares Exp+Mat40, PC+Mat40, Hybrid+Mat40 and Material with 20 trials per pair.
Round 10 explores recombinations under Mat40.
Round 11 and Round 12 provide AB/ID/MCTS comparison for report usage.

