# Assignment 2 - Ataxx Report

## 1. Introduction

Ataxx is a two-player strategy board game that is deterministic, perfect information, and zero-sum. The game is played on a 7x7 grid, labeled with coordinates from (0,0) to (6,6).

![](../ataxx/images/1.png)

Initially, P1 starts with two pieces at (0,0) and (6,6), while P2 starts with two pieces at (0,6) and (6,0). Players take turns to take either of the following actions:

- **Clone**: Move a piece to an empty adjacent (8-neighbor) cell. The original piece remains in place and a new piece is created.
- **Jump**: Move a piece exactly 2 squares away, but only vertically or horizontally. The original piece is relocated.

![](../ataxx/images/2.png)

The capture mechanism: After each move, all opponent pieces in the surrounding 8-neighborhood of the destination cell are converted to the current player's pieces.

![](../ataxx/images/3.png)

The game terminates when any of the following conditions are met:

1. Board is full.
2. A player has no pieces left. The opponent wins.
3. The number of moves reaches 200.

If the game ends due to the board being full or reaching the move limit, the player with more pieces on the board wins. 

In this report, we will discuss the implementation of an Ataxx agent using various search algorithms and heuristics, and evaluate the performance of different strategies.

## 2. Search Framework

### 2.1. Minimax Search

The search space can be represented as a tree where each node corresponds to a game state, and edges represent possible moves. 

The Minimax algorithm is used to explore this tree, where the maximizing player (our agent) tries to maximize the advantage, and the minimizing player (the opponent) tries to minimize it. The root node is a max node (our turn), and the child nodes alternate between min and max nodes.

The following image illustrates the idea of the Minimax algorithm (where green nodes are max nodes and red nodes are min nodes).

![](../breakthrough/images/r1.png)

Consider the complexity of the search space. The branching factor (number of possible moves) can be quite large, and the maximum branching factor is at least 128 (when P1 has 16 pieces and each can move to 8 adjacent cells). The depth of the search tree can be up to 200 (the move limit). Therefore, the time complexity of a naive Minimax search is $O(b^d)$, where $b$ is the branching factor and $d$ is the depth of the tree. It is computationally infeasible to search the entire tree, so optimizations such as alpha-beta pruning and heuristic evaluation are necessary.

For heuristic evaluation, a maximum search depth `maxDepth` is given. If the search reaches this depth, the game state is evaluated using a heuristic function. The heuristic function first check for terminal states (win/loss), and if the game is not terminal, it estimates the advantage of the current player based on various factors that are going to be discussed in Section 4.

Mathematically, the Minimax value of a node can be defined as follows:

$$
V(s, d, \text{maximizing}) =
\begin{cases}
\text{Heuristic}(s) & \text{if } d = \text{maxDepth} \\
\max_{a \in \text{Actions}(s)} V(\text{Result}(s, a), d+1, \text{False}) & \text{if maximizing} \\
\min_{a \in \text{Actions}(s)} V(\text{Result}(s, a), d+1, \text{True}) & \text{if not maximizing}
\end{cases}
$$

### 2.2. Alpha-Beta Negamax

Alpha-beta pruning is an optimization technique for the Minimax algorithm that reduces the number of nodes evaluated in the search tree. It uses two values, $\alpha$ and $\beta$, to keep track of the best scores for both players.

Negamax is a variant of Minimax that simplifies the implementation by treating both players symmetrically. Due to the zero-sum nature of the game, given a game state, the value for the maximizing player can be represented as the negative of the value for the minimizing player. Therefore, during the recursive calls, the returned value is negated to switch perspectives.

The following image illustrates the idea of the Alpha-beta Negamax algorithm (where $[\alpha, \beta]$ represents the current bounds for pruning, and gray nodes are pruned).

![](../breakthrough/images/r2.png)

The alpha-beta negamax algorithm can be defined as follows:

$$
V(s, d, \alpha, \beta) =
\begin{cases}
\text{Heuristic}(s) & \text{if } d = \text{maxDepth} \\
\max_{a \in \text{Actions}(s)} (-V(\text{Result}(s, a), d+1, -\beta, -\alpha)) & \text{otherwise}
\end{cases}
$$

In Alpha-beta Negamax, a beta cut-off occurs when the value of a node is greater than or equal to $\beta$, which means that the opponent will avoid this branch, because a better move for them has already been found. If a move is not pruned but greater than $\alpha$, $\alpha$ is updated to this new value. Finally, the algorithm returns the $\alpha$ value, which represents the best score for the current player.

Alpha-beta Negamax efficiently reduces the number of nodes evaluated, but it still requires a maximum search depth to be set, and the performance can vary greatly depending on the branching factor and the quality of the heuristic evaluation.

### 2.3. Iterative Deepening

To maximize the search depth within a time limit, the iterative deepening technique is used. This approach performs a depth-limited search repeatedly, increasing the depth limit with each iteration until the time limit is reached.

Specifically, the Alpha-beta Negamax search is first called with a depth limit of 1. If the search completes within the time limit, the depth limit is increased by 1, and the search is performed again. This process continues until the time limit is exceeded. The best move found in the last completed search is returned as the final move.

The advantage of iterative deepening is that it allows the agent to find a reasonable move even if the time limit is reached before the maximum depth is fully explored. It also helps to improve move ordering, where the best moves found in earlier iterations can be prioritized in later iterations, increasing the chances of pruning in the alpha-beta search. With advanced move ordering techniques (which will be discussed in Section 3), the search can be significantly more efficient, allowing to reach deeper levels compared to a fixed-depth search in the same time limit.

### 2.4. Monte Carlo Tree Search (MCTS)

In addition to knowledge-based search algorithms, the Monte Carlo Tree Search (MCTS) algorithm is also implemented. It is a stochastic search method that builds a search tree based on random simulations of the game. MCTS consists of four main steps: selection, expansion, simulation, and backpropagation.

1. **Selection**: Starting from the root node, the algorithm selects the best child node at each level based on a selection policy (UCT) until it reaches a leaf node.
2. **Expansion**: If the leaf node is not a terminal state, one of the possible moves is added as a child node.
3. **Simulation**: Simulate a random playout from the new child node until a terminal state is reached, and determine the winner.
4. **Backpropagation**: Update the statistics (win/visit counts) of the nodes along the path from the new child node back to the root based on the simulation result.

The UCT (Upper Confidence Bound for Trees) selection policy is defined as follows:

$$
\text{UCT}(s, a) = \frac{w(s, a)}{n(s, a)} + K \sqrt{\frac{\log N(s)}{n(s, a)}}
$$

Where:

- $w(s, a)$ is the number of wins for action $a$ from state $s$.
- $n(s, a)$ is the number of visits for action $a$ from state $s$.
- $N(s)$ is the total number of visits for state $s$.
- $K = \sqrt{2}$ is the exploration constant.

The UCT formula balances exploration (choosing less visited nodes) and exploitation (choosing nodes with higher win rates). MCTS can be particularly effective in games with large branching factors and less predictable outcomes, and does not require any prior knowledge about the game.

## 3. Optimization Techniques

Several optimization techniques are necessary to increase the efficiency of the search algorithms. They either reduce the constant factors in the time complexity or reduce the number of nodes explored.

### 3.1. Bitboard Representation

A bitboard representation compresses the game state into bit strings. For this game, two 64-bit integers are used to represent the positions of P1 and P2 pieces on the 7x7 board, respectively.

The board coordinate $(r, c)$ is mapped to the bit index $i$ in the bitboard as follows: $i = 8 \cdot r + c$. For example, the initial position of P1 with pieces at (0,0) and (6,6) can be represented as a bitboard where 0 and 48 bits are set to 1, and the rest are 0.

Bitboard representation allows for several further optimizations:

- During recursive calls, instead of creating new arrays for the game state, we can simply pass the bitboards by value. This is both memory efficient and faster.
- Lowbit can be used to iterate through the pieces of a player efficiently. For example, to find all pieces of P1, we can use a loop that repeatedly extracts the least significant bit (LSB) from the bitboard until it becomes zero.

```cpp
inline int lsb_index(U64 x) {
    return __builtin_ctzll(x);
}

BitBoard s; // current game state
U64 t = s.p1;
while (t) {
    int idx = lsb_index(t);
    t &= t - 1; // remove the LSB
    // process the piece at index idx
}
```

- Popcount can be used to count the number of pieces for a player efficiently.

```cpp
inline int popcount(U64 x) {
    return __builtin_popcountll(x);
}

BitBoard s; // current game state
int p1_count = popcount(s.p1);
```

### 3.2. Precomputed Masks

To check for valid positions and moves efficiently, the following precomputed masks are used:

- `g_valid_mask`: The mask with bits set to 1 for valid board positions (0 to 6, 8 to 14, ..., 48 to 54).
- `g_adj_mask`: A list of masks for each position that indicate the 8-neighbor cells for capture and clone moves.
- `g_jump_dst_mask`: A list of masks for each position that indicate the valid jump destinations.

When used, bitwise ANDed with the bitboard to quickly determine valid positions and moves:

- Clone targets: `g_adj_mask[idx] & empty_cells`
- Jump targets: `g_jump_dst_mask[idx] & empty_cells`
- Converted pieces after a move: `g_adj_mask[dst_idx] & opponent_pieces`

This simplifies the capture logic:

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

### 3.3. Zobrist Hashing and Transposition Table

Zobrist hashing is a technique to generate a unique hash value for each game state. It uses a precomputed random number for each possible piece and position, and the hash value for any state can be computed by XORing the corresponding random numbers for the piece-position combinations.

In Ataxx, a total of $NM = 49$ positions and 2 piece types (P1 and P2) result in $2 \cdot NM = 98$ random numbers in `g_zob`. In addition, `g_zob_side` is used to indicate which player's turn it is; because even if the positions are the same, the game state is different when it's P1's turn versus P2's turn. After a move is made, `g_zob_side` is XORed to the hash value to reflect the change in the player's turn; due to the properties of XOR that $A \oplus \text{g\_zob\_side} \oplus \text{g\_zob\_side} = A$, when it is applied again and P1's turn is back, the hash value will be the same as before.

Zobrist hashing allows for incremental updates to the hash value when a move is made, which is much faster than recomputing the hash. When a move is made, find the pieces that have changed (created, removed, or converted) and XOR the corresponding random numbers to update the hash value.

```cpp
inline U64 incremental_hash(U64 old_hash, const Bitboard& old_s, const Bitboard& new_s) {
    U64 h = old_hash ^ g_zob_side;

    U64 p1_diff = old_s.p1 ^ new_s.p1;
    while (p1_diff) {
        int idx = lsb_index(p1_diff);
        p1_diff &= (p1_diff - 1);
        h ^= g_zob[idx][1];
    }

    U64 p2_diff = old_s.p2 ^ new_s.p2;
    while (p2_diff) {
        int idx = lsb_index(p2_diff);
        p2_diff &= (p2_diff - 1);
        h ^= g_zob[idx][2];
    }

    return h;
}
```

Zobrist hashing also enables efficient storage and retrieval of game states in a transposition table, which is a hash table that stores previously evaluated game states and their corresponding heuristic values. When the search algorithm encounters a game state, it can check the transposition table to see if the state has already been evaluated. If it has, the stored heuristic value can be used directly. As there are often multiple paths to reach the same game state, this can significantly reduce the number of nodes evaluated.

The TT entry contains the following information:

- `key`: The full 64-bit hash value of the game state.
- `score`: The heuristic evaluation score of the game state.
- `depth`: The remaining depth of the search (`maxDepth - d`) when the state was evaluated.
- `flag`: Indicates whether the stored score is an exact value, a lower bound, or an upper bound.
- `valid`: A boolean indicating whether the entry is valid, initialized to false.
- `best`: The best move found from this state.

When storing an entry in the transposition table, only the last 19 bits of the hash value are used as the index, and the full 64-bit hash is stored in the `key` field to handle collisions: 

- When retrieving an entry, only if the entry is valid and the stored key matches the current hash value, the entry is considered a hit and returned.
- When storing an entry, it will replace the existing entry if it is invalid, or if it happens to have the same key, or if it has a shallower remaining depth than the new entry. This way, deeper evaluations are prioritized in the transposition table.

```cpp
inline TTEntry* tt_probe(U64 key) {
    TTEntry* e = &g_tt[key & TT_MASK];
    if (e->valid && e->key == key) return e;
    return nullptr;
}

inline void tt_store(U64 key, int8 depth, int8 flag, int score, const Move& best) {
    TTEntry& e = g_tt[key & TT_MASK];
    if (!e.valid || e.key == key || e.depth <= depth) {
        e.valid = 1;
        e.key = key;
        e.depth = depth;
        e.flag = flag;
        e.score = score;
        e.best = best;
    }
}
```

### 3.4. Move Ordering Heuristic (Killer Moves)

As previously mentioned, the efficiency of alpha-beta pruning can be significantly improved by ordering the moves such that the best moves are evaluated first.

Killer moves are moves that have caused a beta cut-off in the past at the same depth. The idea is that if a move has previously caused a cut-off, it is likely to be a good move in similar positions, and should be tried early in the move ordering. 

At each depth, the two most recent killer moves are stored in a `killer` array. When a beta cut-off occurs, the move that caused the cut-off is stored as the first killer move for that depth, and the previous first killer move (if any) is moved to the second slot.

The following move ordering is used when generating moves for a given game state:

1. TT best move
2. `killer[depth][0]`
3. `killer[depth][1]`
4. Other moves

For other moves, the following heuristic is used to assign a priority score:

$$
V(mv) = \text{base} + \text{cap} \cdot 30 - \text{dist}((3, 3), (dr, dc))
$$

Where:

- `base` is 300 for clone moves and 100 for jump moves, as clone moves are generally more advantageous.
- `cap` is the number of opponent pieces that would be captured by this move.
- `dist((3, 3), (dr, dc))` is the Euclidean distance from the destination cell to the center of the board, which encourages moves towards the center.

```cpp
void generate_moves(const Bitboard& s, int8 player, vector<Move>& moves, bool any_one = false) {
    // Generate all valid moves for the given player. If any_one is true, return the first valid move found.
    moves.clear();
    U64 me = (player == 1 ? s.p1 : s.p2);
    U64 occ = s.p1 | s.p2;
    U64 empty = g_valid_mask & (~occ);

    while (me) {
        int idx = lsb_index(me);
        me &= (me - 1);
        int sr = idx / 8;
        int sc = idx % 8;

        U64 clone_targets = g_clone_dst_mask[idx] & empty;
        while (clone_targets) {
            int didx = lsb_index(clone_targets);
            clone_targets &= (clone_targets - 1);
            int dr = didx / 8;
            int dc = didx % 8;
            Move mv(sr, sc, dr, dc, 1);

            U64 cap_mask = g_adj_mask[didx] & (player == 1 ? s.p2 : s.p1);
            int cap = popcnt(cap_mask);
            mv.pri = 300 + cap * 30 - center_dist2(dr, dc);
            moves.push_back(mv);
            if (any_one) return;
        }

        U64 jump_targets = g_jump_dst_mask[idx] & empty;
        while (jump_targets) {
            int didx = lsb_index(jump_targets);
            jump_targets &= (jump_targets - 1);
            int dr = didx / 8;
            int dc = didx % 8;
            Move mv(sr, sc, dr, dc, 0);

            U64 cap_mask = g_adj_mask[didx] & (player == 1 ? s.p2 : s.p1);
            int cap = popcnt(cap_mask);
            mv.pri = 100 + cap * 30 - center_dist2(dr, dc);
            moves.push_back(mv);
            if (any_one) return;
        }
    }
}
```

## 4. Heuristic

Choosing an effective heuristic evaluation function is crucial for the performance of the agent.

All the heuristics below are the difference between the current player and the opponent. For simplicity, the discussed formula will be for P1, and the value for P2 can be calculated symmetrically. The final heuristic value will be the value for P1 minus the value for P2.

### 4.1. Material

The most straightforward heuristic is to count the number of pieces for each player. The player with more pieces on the board is generally in a better position. For each piece of P1:

$$
f = 1
$$

The resulting heuristic value is the number of pieces of P1 minus the number of pieces of P2:

$$
F = \#P1 - \#P2
$$

![](../ataxx/images/heuristic/material.png)

### 4.2. Mobility

Mobility counts the number of valid moves available to the player. For each piece of P1:

$$
f = \#\text{clone moves} + \#\text{jump moves}
$$

![](../ataxx/images/heuristic/mobility.png)

### 4.3. Center Control

Controlling the center of the board is often advantageous, as it allows for more mobility and better capture opportunities. For each piece of P1:

$$
f = 50 - 4 \cdot \text{dist}_{L2}^2((3, 3), (r, c))
$$

![](../ataxx/images/heuristic/center_control.png)

### 4.4. Potential Conversion

Potential conversion only cares about the number of opponent pieces that can be captured in the next turn. For each piece of P1:

$$
f = 25 \cdot \#\text{opponent adjacent}
$$

![](../ataxx/images/heuristic/potential_conversion.png)

### 4.5. Infection Pressure

Infection pressure counts both the number of opponent pieces and the number of friendly pieces in the 8-neighborhood of each piece. Enemy pieces provide more capture opportunities, while friendly pieces provide more support and chances for follow-up moves. For each piece of P1:

$$
f = 12 \cdot \#\text{opponent adjacent} + 4 \cdot \#\text{friendly adjacent}
$$

![](../ataxx/images/heuristic/infection_pressure.png)

### 4.6. Expansion

Expansion is Mobility but weighted, giving higher scores to clone moves as they create new pieces and thus have more long-term potential. For each piece of P1:

$$
f = 6 \cdot \#\text{clone moves} + 3 \cdot \#\text{jump moves}
$$

![](../ataxx/images/heuristic/expansion.png)

### 4.7. Safety

Different from Infection pressure which is an offensive heuristic, Safety negatively weights the number of opponent pieces in the 8-neighborhood, as they pose a threat to be captured in the next turn. For each piece of P1:

$$
f = 6 \cdot \#\text{friendly adjacent} - 8 \cdot \#\text{opponent adjacent}
$$

![](../ataxx/images/heuristic/safety.png)

### 4.8. Influence

Influence is a global (not piece-wise) heuristic that considers the level of control and threat for each cell on the board. For each empty cell, it contributes to the player who has more pieces in its 8-neighborhood; the score for that cell is:

$$
g = 9 \cdot (\#P1 {\text{adjacent}} - \#P2 {\text{adjacent}})
$$

![](../ataxx/images/heuristic/influence.png)

### 4.9. Frontier

Frontier counts the number of pieces that are adjacent to at least one empty cell, and provide an early-game bonus and a late-game penalty (the game status is estimated by the total number of pieces on the board. If less than 33, it is early game; otherwise, it is late game).

$$
f = \begin{cases}
6 & \text{if early game} \\
-6 & \text{if late game}
\end{cases}
$$

![](../ataxx/images/heuristic/frontier.png)

### 4.10. Position Weight

A 7x7 table of position weights is used to encourage occupying more advantageous positions on the board.

```cpp
const int PositionWeightHeuristic::pos_weight[7][7] = {
    {90, -20, 10, 5, 10, -20, 90},
    {-20, -50, -5, 0, -5, -50, -20},
    {10, -5, 20, 15, 20, -5, 10},
    {5, 0, 15, 30, 15, 0, 5},
    {10, -5, 20, 15, 20, -5, 10},
    {-20, -50, -5, 0, -5, -50, -20},
    {90, -20, 10, 5, 10, -20, 90}
};
```

The values are determined after experimenting with previous heuristics and observing the game states. The corners are given the highest weight as they are the safest positions, while the center is also valuable for its mobility and influence. The cells adjacent to the corners are heavily penalized as they are vulnerable to being captured, without good opportunities for extending or escaping.

![](../ataxx/images/heuristic/position_weight.png)

### 4.11. Control Area

Control area gives +32 if a piece could move to a 8-neighbor cell and +16 if a piece could jump next turn. This ring of control around a piece approximates the local influence of that piece, and prevents the agent to blocking itself by putting pieces too close to each other.

$$
f = 32 \cdot I_[d=1] + 16 \cdot I_[d=2]
$$

![](../ataxx/images/heuristic/control_area.png)

### 4.12. Aggression

Aggression awards pieces that are close to the opponent, as they have more chances to capture opponent pieces and thus gain a material advantage. For each piece of P1:

$$
f = 8 \cdot (12 - \text{dist}_{L1}((r, c), \text{nearest opponent}))
$$

Manhattan distance is used.

![](../ataxx/images/heuristic/aggression.png)

### 4.13. Hybrid

After preliminary testing the performance of each individual heuristic (to be discussed in Section 5), a hybrid heuristic is created by combining Center control, infection pressure and expansion.

$$
f = 40 - 3 \cdot \text{dist}_{L2}^2((3, 3), (r, c)) + 10 \cdot \#\text{opponent adjacent} + 4 \cdot \#\text{clone moves}
$$

![](../ataxx/images/heuristic/hybrid.png)

## 5. Evaluation

### 5.1. Tournament Setup

To evaluate the performance of different heuristics and search algorithms, a tournament is conducted where each pair of heuristics is tested against each other. By default, each pair of heuristics is tested with 10 trial (swapping sides for every other trial), and the time limit for each move is set to 1000 ms. The results are ranked primarily by the number of wins, then by the number of timeout wins (winning after 200 moves, lower is better), and finally by the average move time (lower is better).

### 5.2. Evaluation of Search Algorithms