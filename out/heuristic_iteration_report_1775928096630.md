# Ataxx Heuristic Iteration Report

Run tag: 1775928096630

Stage 13: base heuristic re-validation after algorithm updates.
Ranking key: wins desc, timeout wins asc, avg move time asc.

## Stage 13 - Base Heuristics Re-Validation

- CSV: out/stage13_base_revalidate_1775928096630.csv
- Format: iterative deepening only, time limit 1000 ms, 10 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Timeout Wins | Avg Move Time (ms) |
|---:|---|---:|---:|---:|
| 1 | Influence | 105 | 25 | 15.11 |
| 2 | Expansion | 90 | 50 | 12.31 |
| 3 | Material | 75 | 25 | 9.93 |
| 4 | PotentialConversion | 70 | 20 | 11.34 |
| 5 | Frontier | 70 | 20 | 12.44 |
| 6 | Mobility | 70 | 20 | 46.44 |
| 7 | PositionWeight | 70 | 35 | 10.52 |
| 8 | Hybrid | 65 | 20 | 19.33 |
| 9 | Aggression | 65 | 25 | 34.23 |
| 10 | Adaptive | 60 | 30 | 18.14 |
| 11 | ControlArea | 50 | 35 | 13.82 |
| 12 | CenterControl | 45 | 5 | 15.28 |
| 13 | InfectionPressure | 45 | 20 | 14.21 |
| 14 | Safety | 30 | 5 | 12.65 |

## Stage 14 - Pairwise Combination Study

- CSV: out/stage14_pairwise_combo_1775928096630.csv
- Format: iterative deepening only, time limit 1000 ms, 10 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Timeout Wins | Avg Move Time (ms) |
|---:|---|---:|---:|---:|
| 1 | Influence+PotentialConversionx1 | 70 | 25 | 22.14 |
| 2 | Influence | 60 | 20 | 17.79 |
| 3 | Influence+Expansionx1 | 60 | 25 | 17.82 |
| 4 | Expansion+PotentialConversionx1 | 50 | 10 | 18.69 |
| 5 | Influence+Frontierx1 | 50 | 10 | 20.20 |
| 6 | Frontier | 50 | 30 | 10.64 |
| 7 | PotentialConversion+Frontierx1 | 50 | 30 | 12.62 |
| 8 | Expansion+Frontierx1 | 40 | 10 | 19.05 |
| 9 | Expansion | 40 | 20 | 9.30 |
| 10 | Material | 40 | 30 | 8.29 |
| 11 | PotentialConversion | 40 | 30 | 9.93 |

## Stage 16 - Top Stage14 Models vs Expansion+Mat40

- CSV: out/stage16_top_stage14_vs_exp40_1775928096630.csv
- Format: iterative deepening only, time limit 1000 ms, 20 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Timeout Wins | Avg Move Time (ms) |
|---:|---|---:|---:|---:|
| 1 | Influence+Expansionx1 | 60 | 30 | 21.48 |
| 2 | Expansion+Mat40 | 50 | 0 | 17.32 |
| 3 | Expansion+PotentialConversionx1 | 50 | 10 | 19.82 |
| 4 | Influence+PotentialConversionx1 | 30 | 0 | 19.94 |
| 5 | Influence | 10 | 0 | 21.80 |

## Stage 17 - Combo/Expansion/MaterialPlus Adjustment

- CSV: out/stage17_combo_expansion_adjust_1775928096630.csv
- Format: iterative deepening only, time limit 1000 ms, 20 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Timeout Wins | Avg Move Time (ms) |
|---:|---|---:|---:|---:|
| 1 | Influence+PotentialConversionx1+Mat-40 | 120 | 30 | 21.81 |
| 2 | Influence+Expansionx1 | 100 | 50 | 22.38 |
| 3 | Influence+PotentialConversionx1+Mat40 | 90 | 0 | 22.21 |
| 4 | Influence+PotentialConversionx1+Mat80 | 80 | 0 | 23.88 |
| 5 | Expansion+Mat40 | 70 | 0 | 19.13 |
| 6 | Influence+PotentialConversionx1 | 70 | 0 | 21.89 |
| 7 | Influence+PotentialConversionx1+Mat0 | 70 | 0 | 22.06 |
| 8 | Influence+PotentialConversionx1+Expansionx1+Mat40 | 70 | 0 | 29.11 |
| 9 | Influence+PotentialConversionx1+Expansionx1 | 50 | 10 | 30.71 |

## Selection Summary

Stage 13 best: **Influence** (wins=105, timeout wins=25, avg move time=15.11 ms).

Stage 14 best: **Influence+PotentialConversionx1** (wins=70, timeout wins=25, avg move time=22.14 ms).
Stage 15: skipped (Expansion+Mat40 kept as strong baseline).
Stage 16 best: **Influence+Expansionx1** (wins=60, timeout wins=30, avg move time=21.48 ms).
Stage 17 best: **Influence+PotentialConversionx1+Mat-40** (wins=120, timeout wins=30, avg move time=21.81 ms).

Rounds 13-14 use iterative deepening with depth=6, time limit 1000ms, 10 trials per pair.
Rounds 16-17 use 20 trials per pair as requested.

