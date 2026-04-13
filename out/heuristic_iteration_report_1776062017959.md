# Ataxx Heuristic Iteration Report

Run tag: 1776062017959

Stage 22: clone base priority sweep for Expansion+Mat40.
Ranking key: wins desc, timeout wins asc, avg move time asc.

## Stage 22 - Clone Base Priority Sweep

- CSV: out/stage22_clone_base_sweep_1776062017959.csv
- Format: iterative deepening only, time limit 1000 ms, 2 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Timeout Wins | Avg Move Time (ms) |
|---:|---|---:|---:|---:|
| 1 | ID-Expansion+Mat40-CloneBase60 | 10 | 2 | 847.02 |
| 2 | ID-Expansion+Mat40-CloneBase100 | 5 | 1 | 850.82 |
| 3 | ID-Expansion+Mat40-CloneBase250 | 4 | 0 | 706.23 |
| 4 | ID-Expansion+Mat40-CloneBase200 | 4 | 0 | 806.32 |
| 5 | ID-Expansion+Mat40-CloneBase150 | 4 | 0 | 885.73 |
| 6 | ID-Expansion+Mat40-CloneBase300 | 3 | 0 | 876.63 |

## Selection Summary

Stage 22 best: **ID-Expansion+Mat40-CloneBase60** (wins=10, timeout wins=2, avg move time=847.02 ms).

Stage 22 compares Expansion+Mat40 under different clone base priority values in generate_moves.
Config: ID only, time limit 1000ms, tested clone base priority = {60,100,150,200,250,300}.

