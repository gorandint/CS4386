# Ataxx Heuristic Iteration Report

Run tag: 1776006159615

Stage 18: AB7/ID comparison with selected strong heuristics.
Ranking key: wins desc, timeout wins asc, avg move time asc.

## Stage 18 - AB7 vs ID Selected Models

- CSV: out/stage18_ab7_id_compare_1776006159615.csv
- Format: iterative deepening only, time limit 1000 ms, 10 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Timeout Wins | Avg Move Time (ms) |
|---:|---|---:|---:|---:|
| 1 | AB7-Influence+PotentialConversionx1+Mat-40 | 25 | 0 | 106.83 |
| 2 | ID-Influence+PotentialConversionx1+Mat-40 | 25 | 10 | 12.49 |
| 3 | AB7-Material | 20 | 0 | 35.31 |
| 4 | AB7-Expansion+Mat40 | 20 | 0 | 92.95 |
| 5 | ID-Expansion+Mat40 | 10 | 0 | 11.32 |

## Selection Summary

Stage 18 best: **AB7-Influence+PotentialConversionx1+Mat-40** (wins=25, timeout wins=0, avg move time=106.83 ms).

Stage 18 compares AB7-Material, AB7/ID-(Expansion+Mat40), and AB7/ID-(Influence+PotentialConversionx1+Mat-40).
Config: iterative deepening/AB as listed, ID depth=6, time limit 1000ms, 10 trials per pair.

