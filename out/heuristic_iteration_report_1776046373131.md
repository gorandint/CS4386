# Ataxx Heuristic Iteration Report

Run tag: 1776046373131

Stage 21: ID model comparison (1500ms, maxDepth field=10000).
Ranking key: wins desc, timeout wins asc, avg move time asc.

## Stage 21 - ID Influence/Expansion/PotentialConversion/MaterialPlus

- CSV: out/stage21_id_combo_materialplus_1776046373131.csv
- Format: iterative deepening only, time limit 1500 ms, 10 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Timeout Wins | Avg Move Time (ms) |
|---:|---|---:|---:|---:|
| 1 | ID-Expansion+Mat40 | 35 | 15 | 1261.44 |
| 2 | ID-Influence+Expansionx1+Mat0 | 26 | 2 | 1258.30 |
| 3 | ID-Influence+PotentialConversionx1+Mat-40 | 20 | 5 | 1060.00 |
| 4 | ID-Influence+Expansionx1+Mat-40 | 11 | 0 | 1031.04 |
| 5 | ID-Influence+PotentialConversionx1+Mat0 | 8 | 5 | 1343.99 |

## Selection Summary

Stage 21 best: **ID-Expansion+Mat40** (wins=35, timeout wins=15, avg move time=1261.44 ms).

Stage 21 compares ID variants: Influence+Expansion with Mat(0/-40), Influence+PotentialConversion with Mat(0/-40), and Expansion+Mat40.
Config: ID only, time limit 1500ms, maxDepth field set to 10000 (kept consistent with submission style).
Note: ID in solver uses time-driven search; maxDepth profile field has no functional impact.

