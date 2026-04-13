# Ataxx Heuristic Iteration Report

Run tag: 1776063997594

Stage 23: fixed clone base=60 heuristic comparison.
Ranking key: wins desc, timeout wins asc, avg move time asc.

## Stage 23 - CloneBase60 Heuristic Comparison

- CSV: out/stage23_clone60_heuristic_compare_1776063997594.csv
- Format: iterative deepening only, time limit 1000 ms, 10 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Timeout Wins | Avg Move Time (ms) |
|---:|---|---:|---:|---:|
| 1 | ID-Expansion+Mat40 | 27 | 5 | 867.19 |
| 2 | ID-Influence+Expansionx1 | 14 | 2 | 911.21 |
| 3 | ID-Material | 12 | 8 | 854.44 |
| 4 | ID-Influence+PotentialConversionx1 | 7 | 0 | 834.08 |

## Selection Summary

Stage 23 best: **ID-Expansion+Mat40** (wins=27, timeout wins=5, avg move time=867.19 ms).

Stage 23 compares four heuristics under fixed clone base priority=60.
Config: ID only, time limit 1000ms, heuristics = {Expansion+Mat40, Influence+Expansion, Influence+PotentialConversion, Material}, trials=10 per pair.

