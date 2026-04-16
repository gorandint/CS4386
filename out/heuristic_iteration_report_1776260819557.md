# Ataxx Heuristic Iteration Report

Run tag: 1776260819557

Stage 25/26: fixed MoveOrdering with additional heuristic tournaments.
Ranking key: wins desc, timeout wins asc, avg move time asc.

## Stage 26 - Top4 Pair Combos + References

- CSV: out/stage26_top4_pair_combos_1776260819557.csv
- Format: iterative deepening only, time limit 1000 ms, 10 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Timeout Wins | Avg Move Time (ms) |
|---:|---|---:|---:|---:|
| 1 | PositionWeight | 58 | 14 | 742.95 |
| 2 | Influence | 55 | 20 | 769.96 |
| 3 | Combo(PositionWeight+PotentialConversion) | 54 | 15 | 791.76 |
| 4 | Combo(PotentialConversion+Mobility) | 50 | 10 | 716.96 |
| 5 | Combo(Influence+PotentialConversion) | 46 | 8 | 754.67 |
| 6 | PotentialConversion | 45 | 20 | 845.59 |
| 7 | Combo(PositionWeight+Influence) | 40 | 5 | 753.73 |
| 8 | Mobility | 40 | 10 | 770.68 |
| 9 | Combo(Influence+Mobility) | 32 | 7 | 760.28 |
| 10 | Combo(PositionWeight+Mobility) | 30 | 0 | 764.07 |

## Selection Summary

Stage 26 best: **PositionWeight** (wins=58, timeout wins=14, avg move time=742.95 ms).

Fixed setting: jump base=100, clone base=120, capture weight=35.
Stage 26 set: top-4 from Stage 25 (as references) + their 6 pairwise APlusB combinations.
For Stage 26 combinations, base heuristics use w_mat=0 and APlusB adds Mat80.
Config: ID only, time limit 1000ms, trials=10 per pair.

