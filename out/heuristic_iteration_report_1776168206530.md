# Ataxx Heuristic Iteration Report

Run tag: 1776168206530

Stage 24: random search on MoveOrdering and Material weight.
Ranking key: wins desc, timeout wins asc, avg move time asc.

## Stage 24 - Random Search + Strong Baselines

- CSV: out/stage24_random_search_1776168206530.csv
- Format: iterative deepening only, time limit 1000 ms, 6 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Timeout Wins | Avg Move Time (ms) |
|---:|---|---:|---:|---:|
| 1 | Expansion_Cl120_Ca35_M80 | 73 | 24 | 739.81 |
| 2 | Expansion_Cl120_Ca10_M30 | 70 | 15 | 796.12 |
| 3 | Expansion_Cl170_Ca15_M40 | 69 | 16 | 763.71 |
| 4 | Expansion_Cl150_Ca50_M60 | 68 | 9 | 794.75 |
| 5 | Expansion_Cl160_Ca50_M90 | 66 | 20 | 850.65 |
| 6 | Expansion_Cl100_Ca25_M80 | 65 | 17 | 814.82 |
| 7 | Expansion_Cl90_Ca40_M70 | 61 | 17 | 869.31 |
| 8 | Expansion_Cl90_Ca45_M80 | 53 | 14 | 915.12 |
| 9 | Expansion_Cl70_Ca40_M100 | 52 | 6 | 844.84 |
| 10 | Expansion_Cl70_Ca30_M80 | 51 | 12 | 889.88 |
| 11 | Expansion_Cl30_Ca50_M50 | 50 | 0 | 826.74 |
| 12 | Expansion_Cl170_Ca30_M100 | 50 | 18 | 801.43 |
| 13 | Expansion_Cl50_Ca20_M40 | 49 | 5 | 861.89 |
| 14 | Expansion_Cl150_Ca25_M100 | 49 | 19 | 849.88 |
| 15 | Influence+Expanison_Cl300_Ca30_M60 | 48 | 12 | 716.64 |
| 16 | Expansion_Cl60_Ca30_M70 | 48 | 13 | 874.31 |
| 17 | Expansion_Cl30_Ca10_M60 | 46 | 18 | 923.30 |
| 18 | Expansion_Cl90_Ca30_M30 | 43 | 0 | 849.66 |
| 19 | Influence+Expansion_Cl60_Ca30_M60 | 15 | 0 | 786.13 |

## Selection Summary

Stage 24 best: **Expansion_Cl120_Ca35_M80** (wins=73, timeout wins=24, avg move time=739.81 ms).

Search space (random on Expansion): clone base in [40,320], capture weight in [10,50], material weight in [20,100].
Fixed setting: jump base = 100.
Comparison includes known strong models: Expansion_Cl60_Ca30_M70, Influence+Expansion_Cl60_Ca30_M60, Influence+Expanison_Cl300_Ca30_M60.
Config: ID only, time limit 1000ms, trials=6 per pair.

