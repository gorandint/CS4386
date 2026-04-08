# Ataxx Heuristic Iteration Report

Run tag: 1775585742903

## Heuristic Ideas

- Material: maximize piece count gap.
- Mobility: favor positions with more legal actions.
- CenterControl: keep pieces near center for flexible expansion.
- InfectionPressure: reward adjacency to enemy stones for conversion potential.
- Expansion: maximize reachable empty cells by clone/jump.
- Safety: reward local friendly support and penalize local enemy pressure.
- Influence: value empty cells by nearby friendly minus enemy neighbors.
- Frontier: dynamically value frontier size by game phase.
- Hybrid: combine material + center + pressure + expansion.

## Stage 1 - Base Heuristics

- CSV: out/stage1_base_1775585742903.csv
- Format: iterative deepening only, time limit 1000 ms, 10 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Avg Move Time (ms) |
|---:|---|---:|---:|
| 1 | Hybrid | 40 | 19.70 |
| 2 | Expansion | 35 | 20.57 |
| 3 | Material | 30 | 9.95 |
| 4 | Mobility | 30 | 57.09 |
| 5 | InfectionPressure | 10 | 17.62 |
| 6 | CenterControl | 5 | 7.42 |

## Stage 2 - Material Boost Variants

- CSV: out/stage2_material_boost_1775585742903.csv
- Format: iterative deepening only, time limit 1000 ms, 10 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Avg Move Time (ms) |
|---:|---|---:|---:|
| 1 | Expansion+Mat120 | 50 | 17.47 |
| 2 | Expansion+Mat80 | 50 | 17.47 |
| 3 | Hybrid | 50 | 21.00 |
| 4 | Material | 45 | 8.59 |
| 5 | Material+Mat120 | 45 | 8.62 |
| 6 | Material+Mat80 | 45 | 8.66 |
| 7 | Expansion | 30 | 18.23 |
| 8 | Hybrid+Mat80 | 25 | 16.13 |
| 9 | Hybrid+Mat120 | 20 | 18.60 |

## Stage 3 - Finalists Re-Iteration

- CSV: out/stage3_finalists_1775585742903.csv
- Format: iterative deepening only, time limit 1000 ms, 10 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Avg Move Time (ms) |
|---:|---|---:|---:|
| 1 | Expansion+Mat120 | 40 | 20.98 |
| 2 | Expansion+Mat80 | 40 | 21.17 |
| 3 | Expansion+Mat120+Mat100 | 40 | 21.20 |
| 4 | Expansion+Mat80+Mat100 | 40 | 21.20 |
| 5 | Material | 35 | 7.51 |
| 6 | Material+Mat100 | 35 | 7.63 |
| 7 | Hybrid+Mat100 | 25 | 17.97 |
| 8 | Hybrid | 25 | 21.86 |

## Final Selection

Best heuristic in this run: **Expansion+Mat120** (wins=40, avg move time=20.98 ms).

Observations:

- Material remains a strong baseline in this variant due to conversion snowballing.
- Mobility and infection-aware terms usually help in midgame tactical fights.
- Material boosting (+Mat100 around top heuristics) is often a robust upgrade.
- Frontier terms are phase-sensitive and can underperform if over-weighted.

## Stage 4 - Targeted Expansion Tuning

- CSV: out/stage4_targeted_1775585742903.csv
- Format: iterative deepening only, time limit 1000 ms, 10 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Avg Move Time (ms) |
|---:|---|---:|---:|
| 1 | Expansion+Mat130 | 70 | 22.26 |
| 2 | Expansion+Mat150 | 70 | 22.27 |
| 3 | Expansion+Mat110 | 70 | 22.28 |
| 4 | Expansion+Mat90 | 70 | 22.41 |
| 5 | Expansion+InfectionPressurex20 | 50 | 21.54 |
| 6 | Material | 45 | 6.09 |
| 7 | Hybrid | 45 | 19.66 |
| 8 | Expansion+InfectionPressurex35 | 45 | 21.51 |
| 9 | Expansion+InfectionPressurex20+Mat100 | 35 | 20.04 |
| 10 | Expansion+InfectionPressurex35+Mat100 | 30 | 18.22 |
| 11 | Expansion | 20 | 13.74 |

## Targeted Tuning Conclusion

Top heuristic in targeted stage: **Expansion+Mat130** (wins=70, avg move time=22.26 ms).

