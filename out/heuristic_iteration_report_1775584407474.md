# Ataxx Heuristic Iteration Report

Run tag: 1775584407474

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

