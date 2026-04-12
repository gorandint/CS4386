# Ataxx Heuristic Iteration Report

Run tag: 1775923688050

Stage 13: base heuristic re-validation after algorithm updates.
Ranking key: wins desc, timeout wins asc, avg move time asc.

## Stage 13 - Base Heuristics Re-Validation

- CSV: out/stage13_base_revalidate_1775923688050.csv
- Format: iterative deepening only, time limit 1000 ms, 10 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Timeout Wins | Avg Move Time (ms) |
|---:|---|---:|---:|---:|
| 1 | Influence | 105 | 25 | 19.93 |
| 2 | Expansion | 90 | 50 | 16.11 |
| 3 | Material | 75 | 25 | 12.75 |
| 4 | PotentialConversion | 70 | 20 | 14.90 |
| 5 | Frontier | 70 | 20 | 16.14 |
| 6 | Mobility | 70 | 20 | 60.23 |
| 7 | PositionWeight | 70 | 35 | 13.82 |
| 8 | Hybrid | 65 | 20 | 25.36 |
| 9 | Aggression | 65 | 25 | 45.48 |
| 10 | Adaptive | 60 | 30 | 24.10 |
| 11 | ControlArea | 50 | 35 | 18.35 |
| 12 | CenterControl | 45 | 5 | 20.07 |
| 13 | InfectionPressure | 45 | 20 | 18.87 |
| 14 | Safety | 30 | 5 | 16.69 |

## Stage 14 - Pairwise Combination Study

- CSV: out/stage14_pairwise_combo_1775923688050.csv
- Format: iterative deepening only, time limit 1000 ms, 10 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Timeout Wins | Avg Move Time (ms) |
|---:|---|---:|---:|---:|
| 1 | Influence+PotentialConversionx100 | 70 | 25 | 23.43 |
| 2 | Influence | 60 | 20 | 19.01 |
| 3 | Influence+Expansionx100 | 60 | 25 | 18.86 |
| 4 | Expansion+PotentialConversionx100 | 50 | 10 | 19.78 |
| 5 | Influence+Frontierx100 | 50 | 10 | 21.46 |
| 6 | Frontier | 50 | 30 | 11.30 |
| 7 | PotentialConversion+Frontierx100 | 50 | 30 | 13.40 |
| 8 | Expansion+Frontierx100 | 40 | 10 | 20.30 |
| 9 | Expansion | 40 | 20 | 9.97 |
| 10 | Material | 40 | 30 | 8.83 |
| 11 | PotentialConversion | 40 | 30 | 10.63 |

## Stage 15 - Influence/Expansion MaterialPlus Sweep

- CSV: out/stage15_infl_exp_materialplus_1775923688050.csv
- Format: iterative deepening only, time limit 1000 ms, 10 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Timeout Wins | Avg Move Time (ms) |
|---:|---|---:|---:|---:|
| 1 | Expansion+Mat40 | 55 | 15 | 18.13 |
| 2 | Influence+Mat80 | 45 | 15 | 18.80 |
| 3 | Expansion+Mat80 | 45 | 25 | 18.86 |
| 4 | Expansion+Mat0 | 40 | 15 | 14.49 |
| 5 | Influence+Mat40 | 35 | 15 | 17.52 |
| 6 | Influence+Mat0 | 30 | 15 | 17.76 |
| 7 | Expansion+Mat-40 | 15 | 10 | 9.97 |
| 8 | Influence+Mat-40 | 15 | 10 | 12.46 |

## Stage 16 - Best of Group 14 vs Group 15

- CSV: out/stage16_best14_vs_best15_1775923688050.csv
- Format: iterative deepening only, time limit 1000 ms, 20 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Timeout Wins | Avg Move Time (ms) |
|---:|---|---:|---:|---:|
| 1 | Expansion+Mat40 | 10 | 0 | 23.04 |
| 2 | Influence+PotentialConversionx100 | 10 | 0 | 26.17 |

## Stage 17 - Combination Plus MaterialPlus

- CSV: out/stage17_combo_plus_materialplus_1775923688050.csv
- Format: iterative deepening only, time limit 1000 ms, 20 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Timeout Wins | Avg Move Time (ms) |
|---:|---|---:|---:|---:|
| 1 | Influence+PotentialConversionx100+Mat-40 | 80 | 30 | 22.92 |
| 2 | Influence+PotentialConversionx100+Mat40 | 50 | 0 | 23.78 |
| 3 | Influence+PotentialConversionx100+Mat80 | 50 | 0 | 25.24 |
| 4 | Influence+PotentialConversionx100 | 40 | 0 | 22.90 |
| 5 | Expansion+Mat40 | 40 | 0 | 23.23 |
| 6 | Influence+PotentialConversionx100+Mat0 | 40 | 0 | 23.32 |

## Selection Summary

Stage 13 best: **Influence** (wins=105, timeout wins=25, avg move time=19.93 ms).

Stage 14 best: **Influence+PotentialConversionx100** (wins=70, timeout wins=25, avg move time=23.43 ms).
Stage 15 best: **Expansion+Mat40** (wins=55, timeout wins=15, avg move time=18.13 ms).
Stage 16 best: **Expansion+Mat40** (wins=10, timeout wins=0, avg move time=23.04 ms).
Stage 17 best: **Influence+PotentialConversionx100+Mat-40** (wins=80, timeout wins=30, avg move time=22.92 ms).

Rounds 13-15 use iterative deepening with depth=6, time limit 1000ms, 10 trials per pair.
Rounds 16-17 use 20 trials per pair as requested.

