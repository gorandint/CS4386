# Ataxx Heuristic Iteration Report

Run tag: 1776244074619

Stage 25/26: fixed MoveOrdering with additional heuristic tournaments.
Ranking key: wins desc, timeout wins asc, avg move time asc.

## Stage 25 - Single Heuristics (Clone120/Capture35)

- CSV: out/stage25_single_heuristics_1776244074619.csv
- Format: iterative deepening only, time limit 1000 ms, 10 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Timeout Wins | Avg Move Time (ms) |
|---:|---|---:|---:|---:|
| 1 | PositionWeight | 50 | 4 | 768.70 |
| 2 | Influence | 35 | 5 | 744.80 |
| 3 | PotentialConversion | 30 | 0 | 840.14 |
| 4 | Material | 30 | 0 | 878.56 |
| 5 | Mobility | 29 | 10 | 798.48 |
| 6 | Expansion | 26 | 5 | 763.25 |
| 7 | Frontier | 10 | 10 | 786.39 |

