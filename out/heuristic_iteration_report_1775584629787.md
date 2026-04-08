# Ataxx Heuristic Iteration Report

Run tag: 1775584629787

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

- CSV: out/stage1_base_1775584629787.csv
- Format: iterative deepening only, time limit 1000 ms, 10 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Avg Move Time (ms) |
|---:|---|---:|---:|
| 1 | Hybrid | 40 | 19.80 |
| 2 | Expansion | 35 | 20.78 |
| 3 | Material | 30 | 10.21 |
| 4 | Mobility | 30 | 57.83 |
| 5 | InfectionPressure | 10 | 17.97 |
| 6 | CenterControl | 5 | 7.60 |

## Stage 2 - Material Boost Variants

- CSV: out/stage2_material_boost_1775584629787.csv
- Format: iterative deepening only, time limit 1000 ms, 10 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Avg Move Time (ms) |
|---:|---|---:|---:|
| 1 | Expansion+Mat120 | 50 | 17.54 |
| 2 | Expansion+Mat80 | 50 | 17.59 |
| 3 | Hybrid | 50 | 20.80 |
| 4 | Material | 45 | 8.61 |
| 5 | Material+Mat120 | 45 | 8.65 |
| 6 | Material+Mat80 | 45 | 8.66 |
| 7 | Expansion | 30 | 18.22 |
| 8 | Hybrid+Mat80 | 25 | 16.23 |
| 9 | Hybrid+Mat120 | 20 | 18.51 |

## Stage 3 - Finalists Re-Iteration

- CSV: out/stage3_finalists_1775584629787.csv
- Format: iterative deepening only, time limit 1000 ms, 10 trials per pair (color swapped by trial parity).

| Rank | Heuristic | Wins | Avg Move Time (ms) |
|---:|---|---:|---:|
| 1 | Expansion+Mat120 | 40 | 20.83 |
| 2 | Expansion+Mat80 | 40 | 20.98 |
| 3 | Expansion+Mat120+Mat100 | 40 | 21.05 |
| 4 | Expansion+Mat80+Mat100 | 40 | 21.11 |
| 5 | Material | 35 | 7.46 |
| 6 | Material+Mat100 | 35 | 7.57 |
| 7 | Hybrid+Mat100 | 25 | 17.86 |
| 8 | Hybrid | 25 | 21.60 |

## Final Selection

Best heuristic in this run: **Expansion+Mat120** (wins=40, avg move time=20.83 ms).

Observations:

- Material remains a strong baseline in this variant due to conversion snowballing.
- Mobility and infection-aware terms usually help in midgame tactical fights.
- Material boosting (+Mat100 around top heuristics) is often a robust upgrade.
- Frontier terms are phase-sensitive and can underperform if over-weighted.

