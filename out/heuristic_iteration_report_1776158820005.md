# Ataxx Heuristic Iteration Report

Run tag: 1776158820005

Stage 24: random search on MoveOrdering and Material weight.
Ranking key: wins desc, timeout wins asc, avg move time asc.

## Stage 24 - Random Search + Strong Baselines

- CSV: out/stage24_random_search_1776158820005.csv
- Format: iterative deepening only, time limit 1000 ms, 4 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Timeout Wins | Avg Move Time (ms) |
|---:|---|---:|---:|---:|
| 1 | Expansion_Cl160_Ca35_M30 | 40 | 4 | 760.11 |
| 2 | Expansion_Cl150_Ca25_M80 | 38 | 14 | 824.16 |
| 3 | Expansion_Cl70_Ca30_M40 | 34 | 8 | 860.17 |
| 4 | Expansion_Cl150_Ca25_M90 | 31 | 13 | 786.65 |
| 5 | Expansion_Cl140_Ca20_M50 | 30 | 2 | 801.41 |
| 6 | Expansion_Cl70_Ca35_M90 | 28 | 4 | 888.39 |
| 7 | Expansion_Cl100_Ca15_M70 | 28 | 8 | 743.56 |
| 8 | Expansion_Cl130_Ca40_M70 | 26 | 8 | 725.81 |
| 9 | Expansion_Cl120_Ca15_M100 | 25 | 6 | 788.66 |
| 10 | Influence+Expanison_Cl300_Ca30_M60 | 20 | 4 | 660.18 |
| 11 | Expansion_Cl40_Ca35_M70 | 20 | 6 | 881.34 |
| 12 | Expansion_Cl70_Ca35_M100 | 19 | 3 | 886.06 |
| 13 | Expansion_Cl60_Ca30_M70 | 19 | 6 | 894.88 |
| 14 | Influence+Expansion_Cl60_Ca30_M60 | 6 | 0 | 748.78 |

## Selection Summary

Stage 24 best: **Expansion_Cl160_Ca35_M30** (wins=40, timeout wins=4, avg move time=760.11 ms).

Search space (random on Expansion): clone base in [40,320], capture weight in [10,50], material weight in [20,100].
Fixed setting: jump base = 100.
Comparison includes known strong models: Expansion_Cl60_Ca30_M70, Influence+Expansion_Cl60_Ca30_M60, Influence+Expanison_Cl300_Ca30_M60.
Config: ID only, time limit 1000ms, trials=6 per pair.

