# Ataxx Heuristic Iteration Report

Run tag: 1775919606897

Stage 13: base heuristic re-validation after algorithm updates.
Ranking key: wins desc, timeout wins asc, avg move time asc.

## Stage 13 - Base Heuristics Re-Validation

- CSV: out/stage13_base_revalidate_1775919606897.csv
- Format: iterative deepening only, time limit 1000 ms, 10 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Timeout Wins | Avg Move Time (ms) |
|---:|---|---:|---:|---:|
| 1 | Influence | 105 | 25 | 15.61 |
| 2 | Expansion | 90 | 50 | 12.48 |
| 3 | Material | 75 | 25 | 11.78 |
| 4 | PotentialConversion | 70 | 20 | 11.46 |
| 5 | Frontier | 70 | 20 | 12.48 |
| 6 | Mobility | 70 | 20 | 47.25 |
| 7 | PositionWeight | 70 | 35 | 10.51 |
| 8 | Hybrid | 65 | 20 | 19.57 |
| 9 | Aggression | 65 | 25 | 34.15 |
| 10 | Adaptive | 60 | 30 | 18.14 |
| 11 | ControlArea | 50 | 35 | 13.36 |
| 12 | CenterControl | 45 | 5 | 15.87 |
| 13 | InfectionPressure | 45 | 20 | 14.67 |
| 14 | Safety | 30 | 5 | 12.98 |

## Selection Summary

Stage 13 best: **Influence** (wins=105, timeout wins=25, avg move time=15.61 ms).

Round 13 re-validates a broad set of base heuristics under current algorithm settings.
Manual follow-up policy: pick top-3 non-Material models, test their pairwise/3-way combinations, and tune MaterialPlus on {-40, 0, 40, 80}.
Config: iterative deepening, depth=6, time limit 1000ms, 10 trials per pair.

