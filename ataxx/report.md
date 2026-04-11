# Assignment 1 - Ataxx Report
## 1. Introduction and Problem Setup

Ataxx is a deterministic, two-player, zero-sum board game. This project uses a 7x7 board with a fixed initial state:

- P1 pieces: (0,0), (6,6)
- P2 pieces: (0,6), (6,0)

Move rules:

1. Clone: move to any adjacent cell (8-neighborhood). Source piece stays.
2. Jump: move exactly 2 squares in orthogonal directions. Source piece disappears.
3. Assimilation: after landing, all adjacent opponent pieces are converted.

Terminal conditions:

- one side has no piece,
- board full,
- two consecutive passes,
- ply reaches 200.

If terminated by board/full/pass/ply cap, winner is decided by piece count (draw allowed).

Compared with Breakthrough, Ataxx has much stronger local tactical volatility because one move can flip a neighborhood. Therefore, good Ataxx AI needs both:

- fast tactical search,
- a heuristic that values growth + conversion potential, not just current material.

## 2. Search Frameworks

### 2.1. Alpha-Beta Negamax

The deterministic core is Negamax with Alpha-Beta pruning:

$$
V(s, d, \alpha, \beta) = \max_{m \in \mathcal{M}(s)} \left( -V(T(s,m), d+1, -\beta, -\alpha) \right)
$$

and prune when $\alpha \ge \beta$.

Implementation highlights:

- terminal scoring via evaluate_terminal,
- TT lookup with bound interpretation (EXACT/LOWER/UPPER),
- killer move update on beta cut.

Code anchors:

- ab_negamax in ataxx.cpp
- ab_solver as fixed-depth root driver

### 2.2. Iterative Deepening (ID)

Iterative deepening repeats depth-limited Alpha-Beta from depth 1 to max depth under time budget:

- if current depth fully finishes, keep that best move,
- if timeout happens mid-depth, return best from last completed depth.

This gives anytime behavior and more stable decisions under strict clock.

Code anchor: iterative_deepening_solver.

### 2.3. MCTS Baseline

MCTS implementation uses standard UCT:

$$
	ext{UCT}(i) = \frac{w_i}{n_i} + C\sqrt{\frac{\ln N}{n_i}}
$$

with:

- selection by max UCT,
- expansion from untried moves,
- playout by random or epsilon-greedy heuristic move,
- backpropagation of result.

In this project, MCTS is mainly used as comparison baseline. Under 1000ms, it is consistently weaker than ID in tournament results.

## 3. Bit Optimization and Engine Engineering

### 3.1. 7x7 on 64-bit Layout

Although game board is 49 cells, state is stored as two 64-bit bitboards:

- p1: P1 occupancy bits
- p2: P2 occupancy bits

Bit index mapping:

$$
idx = r \times 8 + c
$$

Only cells with $0 \le r,c < 7$ are valid; others are masked out by valid_mask.

Benefits:

- piece iteration by lowbit: $x \leftarrow x\ \&\ (x-1)$,
- piece count by popcount,
- compact copy for deep search recursion.

### 3.2. Precomputed Masks

During initialization, the engine precomputes per-cell masks:

- adj_mask[idx]: 8-neighborhood (for assimilation and local features)
- clone_dst_mask[idx]: legal clone destinations
- jump_dst_mask[idx]: legal jump destinations

This turns move generation and feature extraction into bitwise operations:

- clone targets: clone_dst_mask[idx] & empty
- jump targets: jump_dst_mask[idx] & empty
- converted pieces after move: adj_mask[dst] & opponent

### 3.3. Move Generation and Application

Move generation scans all pieces of side-to-move, then enumerates clone targets and jump targets from masks.

Priority is assigned online for ordering:

- base preference clone > jump,
- capture potential (number of adjacent enemy pieces after landing),
- center bias penalty by distance.

Move application is branchless-friendly and compact:

1. place destination piece,
2. remove source if jump,
3. compute flip mask by adj_mask[dst] & opponent,
4. transfer flipped bits,
5. update ply and pass count.

### 3.4. Zobrist Hashing and TT

State hash includes:

- piece placement keys,
- side-to-move key,
- ply key,
- pass-count key.

TT entry fields:

- key, score,
- remaining depth,
- bound flag,
- best move.

Lookup policy:

- exact hit can return directly,
- lower/upper bound can tighten alpha/beta,
- cutoff when tightened window collapses.

Store policy prefers deeper or same-key entries.

### 3.5. Killer Moves + TT Move First

At each depth, two killer moves are cached. Ordering applies strong bonuses in this sequence:

1. TT best move
2. killer[depth][0]
3. killer[depth][1]

Empirically this raises cutoff rate and is important for ID depth growth under fixed time.

### 3.6. Time Management

Search checks time every fixed number of visited nodes (bit-mask stride). Once timeout is detected, recursion returns quickly via global flag.

This avoids uncontrolled overrun and keeps tournament runtime predictable.

### 3.7. Key Implementation Snippets

Lowbit iteration and coordinate decode:

```cpp
while (bits) {
  int idx = __builtin_ctzll(bits);
  bits &= (bits - 1);
  int r = idx / 8;
  int c = idx % 8;
  // process (r, c)
}
```

Move generation with precomputed masks:

```cpp
U64 clone_targets = g_clone_dst_mask[idx] & empty;
U64 jump_targets  = g_jump_dst_mask[idx]  & empty;
```

Assimilation after placing destination:

```cpp
U64 flip = g_adj_mask[didx] & (player == 1 ? ns.p2 : ns.p1);
if (player == 1) {
  ns.p2 &= ~flip;
  ns.p1 |= flip;
} else {
  ns.p1 &= ~flip;
  ns.p2 |= flip;
}
```

TT bound usage in Alpha-Beta:

```cpp
if (entry->depth >= remain) {
  if (entry->flag == TT_EXACT) return entry->score;
  if (entry->flag == TT_LOWER) alpha = max(alpha, entry->score);
  else if (entry->flag == TT_UPPER) beta = min(beta, entry->score);
  if (alpha >= beta) return entry->score;
}
```

## 4. Heuristic Design with Diagram-Guided Rationale

All heuristics are P1-centric in estimate(state), then sign-flipped for side-to-move evaluation. Base material term is:

$$
50 \cdot (\#P1 - \#P2)
$$

Below, each heuristic is paired with generated figure (from images/heuristic).

### 4.1. Material

Formula:

$$
f = \#P1
$$

Rationale: direct objective surrogate; stable and low-variance.

Figure: ![](images/heuristic/material.png)

### 4.2. Mobility

Formula:

$$
f = |CloneMoves| + |JumpMoves|
$$

Rationale: more legal options improve tactical flexibility; in Ataxx this correlates with future growth and avoidance of forced pass.

Figure: ![](images/heuristic/mobility.png)

### 4.3. Center Control

Formula:

$$
f = \sum (50 - 4((r-3)^2 + (c-3)^2))
$$

Rationale: center pieces have denser reachable neighborhoods and better conversion reach in multiple directions.

Figure: ![](images/heuristic/center_control.png)

### 4.4. Infection Pressure

Formula:

$$
f = 12 \cdot enemyAdj + 4 \cdot friendAdj
$$

Rationale: enemy adjacency means immediate conversion opportunities; friend adjacency stabilizes local group and follow-up flips.

Figure: ![](images/heuristic/infection_pressure.png)

### 4.5. Expansion

Formula:

$$
f = 6\cdot|CloneReach| + 3\cdot|JumpReach|
$$

Rationale: Ataxx is strongly growth-driven. Clone reach directly predicts one-ply piece increase and territory fill speed; jump reach is useful but less valuable than clone.

Figure: ![](images/heuristic/expansion.png)

### 4.6. Safety

Formula:

$$
f = 6\cdot friendAdj - 8\cdot enemyAdj
$$

Rationale: surrounded stones are fragile and may trigger opponent chain conversions; friend support reduces collapse risk.

Figure: ![](images/heuristic/safety.png)

### 4.7. Influence

Formula:

$$
f(E)=9\cdot(P1\_adj(E)-P2\_adj(E))
$$

Rationale: scores latent control on empty cells, not just occupied cells, improving lookahead quality when board is sparse.

Figure: ![](images/heuristic/influence.png)

### 4.8. Frontier

Formula:

$$
f = 6\cdot frontierCount \quad (early\ game)
$$

Rationale: frontier pieces contact empties and can expand. Early game rewards frontier; late game can reverse this preference in some variants.

Figure: ![](images/heuristic/frontier.png)

### 4.9. Position Weight

Formula:

$$
f = w[r][c]
$$

Rationale: board geometry prior (corners/central belts) can encode manually observed strategic value.

Figure: ![](images/heuristic/position_weight.png)

### 4.10. Potential Conversion

Formula:

$$
f = 25\cdot enemyAdj
$$

Rationale: directly estimates immediate assimilation gain if tactical contact is exploited.

Figure: ![](images/heuristic/potential_conversion.png)

### 4.11. Control Area

Formula:

$$
f = \sum(32\cdot I[d=1] + 16\cdot I[d=2])
$$

Rationale: near-ring control around each piece approximates local reachability field.

Figure: ![](images/heuristic/control_area.png)

### 4.12. Aggression

Formula:

$$
f=(12-nearestDist)\cdot 8
$$

Rationale: encourages pressure and contact against nearest opponent clusters; useful when needing tactical forcing.

Figure: ![](images/heuristic/aggression.png)

### 4.13. Hybrid

Formula:

$$
f=(40-3d^2) + 10\cdot enemyAdj + 4\cdot cloneReach
$$

Rationale: combines center, pressure, and growth in one scoring rule; acts as strong all-round baseline.

Figure: ![](images/heuristic/hybrid.png)

### 4.14. Recombined Families and MaterialBoost

Later-stage candidates:

- CenterExpansion
- CenterPressurePC
- PressureExpansion
- MaterialBoost(base, w)

MaterialBoost adds explicit material correction:

$$
f_{boost}(s)=f_{base}(s)+w\cdot(\#P1-\#P2)
$$

This is crucial because pure shape/activity features may over-extend in tactical positions where material lead should dominate.

## 5. Experimental Protocol

All key results are from structured run tag 1775638626415.

Artifacts:

- report: out/heuristic_iteration_report_1775638626415.md
- csv stage9: out/stage9_core_compare_1775638626415.csv
- csv stage10: out/stage10_recombine_mat40_1775638626415.csv
- csv stage11: out/stage11_ab_vs_id_material_1775638626415.csv
- csv stage12: out/stage12_id_vs_mcts_1775638626415.csv

Shared settings:

- board: 7x7 Ataxx
- ranking: wins desc -> timeout wins asc -> avg move time asc
- timeout-aware evaluation included (ply=200 regarded as timeout-end games)

## 6. Results and Analysis

### 6.1. Stage 9: Core Compare (20 trials/pair)

Candidates:

- Material
- Expansion+Mat40
- PotentialConversion+Mat40
- Hybrid+Mat40

Observed ranking:

1. Expansion+Mat40: 40 wins, 0 timeout wins, 24.38ms
2. Hybrid+Mat40: 40 wins, 20 timeout wins, 28.93ms
3. Material: 20 wins, 10 timeout wins, 13.64ms
4. PotentialConversion+Mat40: 20 wins, 10 timeout wins, 16.74ms

Analysis:

- Expansion+Mat40 and Hybrid+Mat40 have equal win count, but Expansion+Mat40 is clearly better in timeout profile.
- This indicates Expansion gains are not from dragging games to ply cap; they are more decisive wins.

### 6.2. Stage 10: Recombination Under Mat40

Candidates:

- Material
- Expansion+Mat40
- PotentialConversion+Mat40
- Hybrid+Mat40
- CenterExpansion+Mat40
- CenterPressurePC+Mat40
- PressureExpansion+Mat40

Observed ranking top lines:

- Expansion+Mat40: 80 wins, 10 timeout wins, 28.98ms
- PressureExpansion+Mat40: 80 wins, 30 timeout wins, 30.81ms
- Hybrid+Mat40: 70 wins, 20 timeout wins, 27.38ms

Analysis:

- Adding pressure to expansion keeps high win rate, but timeout behavior worsens.
- Expansion+Mat40 gives best overall balance (strength + decisiveness + speed).

### 6.3. Stage 11: AB vs ID (Material-only)

Candidates:

- AB5-Material
- AB6-Material
- AB7-Material
- ID-Material

Results:

- AB7-Material strongest in win count (25), but slower (50.95ms).
- ID-Material has lower wins (15) and timeout wins (10), but good latency (13.25ms).

Analysis:

- fixed deep AB can be tactically powerful in controlled setting,
- ID still preferred for practical clock control and robustness across varying branching patterns.

### 6.4. Stage 12: ID vs MCTS

Candidates:

- ID-Hybrid
- MCTS-Random
- MCTS-Hybrid
- MCTS-Material

Results:

- ID-Hybrid: 30 wins, 42.06ms
- MCTS-Material: 15 wins, 945.05ms
- MCTS-Hybrid: 11 wins, 950.19ms
- MCTS-Random: 4 wins, 979.98ms

Analysis:

- Under 1000ms cap, MCTS spends most budget in simulation and underperforms deterministic search with strong heuristic.
- ID remains dominant in this environment.

## 7. Final Model Decision

Final deployed policy (non-LOCAL path):

- heuristic: Expansion + MaterialBoost(40)
- search: iterative deepening
- runtime parameters: depth cap 10, time limit 1850ms

Reasoning summary:

1. Expansion family consistently wins across core and recombination stages.
2. +Mat40 stabilizes objective alignment when tactical shape and raw count conflict.
3. ID provides strong anytime quality and better practical behavior than fixed-depth AB or MCTS under clock.

## 8. Limitations and Future Work

Current limitations:

- heuristic weights are manually designed (not learned),
- no endgame tablebase,
- no explicit opening book,
- TT replacement policy is simple and could be improved.

Potential improvements:

1. Phase-aware dynamic weighting (opening/mid/endgame schedules).
2. Lightweight learned value prior to complement hand-crafted features.
3. Better MCTS variant (progressive bias or implicit minimax backup).
4. Stronger TT replacement and aspiration windows for deeper ID iterations.

## Appendix A. Code and Artifacts

- engine: ataxx/ataxx.cpp
- heuristic figure generator: ataxx/plot_heuristic_diagrams.py
- figures:
  - images/heuristic/material.png
  - images/heuristic/mobility.png
  - images/heuristic/center_control.png
  - images/heuristic/infection_pressure.png
  - images/heuristic/expansion.png
  - images/heuristic/safety.png
  - images/heuristic/influence.png
  - images/heuristic/frontier.png
  - images/heuristic/position_weight.png
  - images/heuristic/potential_conversion.png
  - images/heuristic/control_area.png
  - images/heuristic/aggression.png
  - images/heuristic/hybrid.png
- latest structured report: out/heuristic_iteration_report_1775638626415.md
- latest csv:
  - out/stage9_core_compare_1775638626415.csv
  - out/stage10_recombine_mat40_1775638626415.csv
  - out/stage11_ab_vs_id_material_1775638626415.csv
  - out/stage12_id_vs_mcts_1775638626415.csv
