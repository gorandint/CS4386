# Ataxx Heuristic Iteration Report

Run tag: 1776006875772

Stage 19-20: AB-depth and AB7-vs-ID verification after fix.
Ranking key: wins desc, timeout wins asc, avg move time asc.

## Stage 19 - Expansion AB5/6/7 vs ID

- CSV: out/stage19_expansion_ab_depth_vs_id_1776006875772.csv
- Format: iterative deepening only, time limit 1000 ms, 10 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Timeout Wins | Avg Move Time (ms) |
|---:|---|---:|---:|---:|
| 1 | ID-Expansion+Mat40 | 28 | 5 | 805.85 |
| 2 | AB7-Expansion+Mat40 | 20 | 0 | 107.25 |
| 3 | AB6-Expansion+Mat40 | 7 | 2 | 22.54 |
| 4 | AB5-Expansion+Mat40 | 5 | 5 | 5.64 |

## Stage 20 - Strong Models AB7 vs ID

- CSV: out/stage20_strong_models_ab7_vs_id_1776006875772.csv
- Format: iterative deepening only, time limit 1000 ms, 10 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Timeout Wins | Avg Move Time (ms) |
|---:|---|---:|---:|---:|
| 1 | ID-Influence+Expansionx1 | 101 | 9 | 806.52 |
| 2 | ID-Influence+PotentialConversionx1+Expansionx1 | 91 | 0 | 794.36 |
| 3 | ID-Influence+PotentialConversionx1 | 91 | 11 | 751.91 |
| 4 | ID-Expansion+Mat40 | 90 | 15 | 837.05 |
| 5 | ID-Frontier | 85 | 13 | 827.66 |
| 6 | ID-Material | 82 | 3 | 837.49 |
| 7 | ID-Influence+PotentialConversionx1+Mat-40 | 75 | 5 | 817.18 |
| 8 | AB7-Expansion+Mat40 | 50 | 5 | 84.39 |
| 9 | AB7-Influence+PotentialConversionx1 | 50 | 5 | 97.98 |
| 10 | AB7-Material | 50 | 15 | 37.34 |
| 11 | AB7-Influence+PotentialConversionx1+Expansionx1 | 45 | 5 | 134.70 |
| 12 | AB7-Frontier | 40 | 2 | 47.40 |
| 13 | AB7-Influence+Expansionx1 | 35 | 0 | 122.99 |
| 14 | AB7-Influence+PotentialConversionx1+Mat-40 | 25 | 0 | 147.42 |

## Selection Summary

Stage 19 best: **ID-Expansion+Mat40** (wins=28, timeout wins=5, avg move time=805.85 ms).
Stage 20 best: **ID-Influence+Expansionx1** (wins=101, timeout wins=9, avg move time=806.52 ms).

Stage 19 compares AB5/6/7 and ID on Expansion+Mat40.
Stage 20 compares AB7 and ID versions of selected strong models from rounds 14-17.
Config: AB depth as listed, ID time-only (maxDepth ignored), time limit 1000ms, 10 trials per pair.

