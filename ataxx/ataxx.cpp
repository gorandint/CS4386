#ifndef LOCAL
#include "battle_base.h"
#endif

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <random>
#include <utility>
#include <vector>

using namespace std;

#ifndef MAX_M
#define MAX_M 7
#define MAX_N 7
#define MAX_PIECES 49
#endif

typedef unsigned long long U64;
typedef char int8;
typedef int16_t int16;

const int WIN_SCORE = 100000000;
const int INF_SCORE = 200000000;
const int MAX_STEPS = 200;
const int MAX_DEPTH = 64;

/* Move */

struct Move {
    int8 sr;
    int8 sc;
    int8 dr;
    int8 dc;
    int8 is_clone;
    int pri;

    Move() : sr(0), sc(0), dr(0), dc(0), is_clone(1), pri(0) {}
    Move(int8 _sr, int8 _sc, int8 _dr, int8 _dc, int8 _is_clone)
        : sr(_sr), sc(_sc), dr(_dr), dc(_dc), is_clone(_is_clone), pri(0) {}
};

inline bool operator==(const Move& a, const Move& b) {
    return a.sr == b.sr && a.sc == b.sc && a.dr == b.dr && a.dc == b.dc && a.is_clone == b.is_clone;
}

inline bool operator<(const Move& a, const Move& b) {
    return a.pri > b.pri;
}

/* Bitboard */

struct Bitboard {
    U64 p1;
    U64 p2;
    int16 ply;
    int8 pass_count;

    Bitboard() : p1(0ULL), p2(0ULL), ply(0), pass_count(0) {}
};

/* Transposition Table */

enum TTFlag { TT_EXACT = 0, TT_LOWER = 1, TT_UPPER = 2 };

struct TTEntry {
    U64 key;
    int score;
    int8 depth;
    int8 flag;
    int8 valid;
    Move best;

    TTEntry() : key(0ULL), score(0), depth(0), flag(0), valid(0), best() {}
};

const int TT_SIZE = 1 << 19;
const int TT_MASK = TT_SIZE - 1;
TTEntry g_tt[TT_SIZE];
Move g_killer[MAX_DEPTH][2];

/* Mask for checking valid positions and move generation */

U64 g_valid_mask = 0ULL; // (0,0) to (6,6)
array<U64, 64> g_adj_mask; // 8-directional adjacent squares for each position
array<U64, 64> g_clone_dst_mask; // Clone destination squares for each position
array<U64, 64> g_jump_dst_mask; // Jump destination squares for each position

/* Zobrist Hashing */

U64 g_zob[64][3];
U64 g_zob_side;
U64 g_zob_ply[MAX_STEPS + 1];
U64 g_zob_pass[3];
bool g_zob_ready = false;

/* Search timeout */

bool g_time_out = false;
chrono::steady_clock::time_point g_start_time;
double g_time_limit_ms = 0.0;

mt19937_64 g_rng((uint64_t)chrono::steady_clock::now().time_since_epoch().count());

/* Bit operations */

inline int lsb_index(U64 x) {
    // Get the index of the least significant set bit (lowest 1)
    return __builtin_ctzll(x);
}

inline int popcnt(U64 x) {
    // Count the number of 1 bits
    return __builtin_popcountll(x);
}

inline U64 bit_at(int r, int c) {
    return 1ULL << (r * 8 + c);
}

inline int8 piece_at(const Bitboard& s, int r, int c) {
    U64 b = bit_at(r, c);
    if (s.p1 & b) return 1;
    if (s.p2 & b) return 2;
    return 0;
}

inline int center_dist2(int r, int c) {
    // Get squared distance to center (3,3)
    int dr = r - 3;
    int dc = c - 3;
    return dr * dr + dc * dc;
}

/* Mask, TT, Killer Initialization */

void init_masks() {
    g_valid_mask = 0ULL;
    for (int r = 0; r < 7; r++) {
        for (int c = 0; c < 7; c++) {
            g_valid_mask |= bit_at(r, c);
        }
    }

    g_adj_mask.fill(0ULL);
    g_clone_dst_mask.fill(0ULL);
    g_jump_dst_mask.fill(0ULL);

    for (int r = 0; r < 7; r++) {
        for (int c = 0; c < 7; c++) {
            int idx = r * 8 + c;
            U64 adj = 0ULL;
            U64 clone_dst = 0ULL;
            U64 jump_dst = 0ULL;

            for (int dr = -1; dr <= 1; dr++) {
                for (int dc = -1; dc <= 1; dc++) {
                    if (dr == 0 && dc == 0) continue;
                    int nr = r + dr;
                    int nc = c + dc;
                    if (nr >= 0 && nr < 7 && nc >= 0 && nc < 7) {
                        U64 b = bit_at(nr, nc);
                        adj |= b;
                        clone_dst |= b;
                    }
                }
            }

            static const int jdr[4] = {-2, 2, 0, 0};
            static const int jdc[4] = {0, 0, -2, 2};
            for (int k = 0; k < 4; k++) {
                int nr = r + jdr[k];
                int nc = c + jdc[k];
                if (nr >= 0 && nr < 7 && nc >= 0 && nc < 7) {
                    jump_dst |= bit_at(nr, nc);
                }
            }

            g_adj_mask[idx] = adj;
            g_clone_dst_mask[idx] = clone_dst;
            g_jump_dst_mask[idx] = jump_dst;
        }
    }
}

void init_zobrist() {
    if (g_zob_ready) return;
    for (int i = 0; i < 64; i++) {
        g_zob[i][0] = 0ULL;
        g_zob[i][1] = g_rng();
        g_zob[i][2] = g_rng();
    }
    g_zob_side = g_rng();
    for (int i = 0; i <= MAX_STEPS; i++) g_zob_ply[i] = g_rng();
    for (int i = 0; i < 3; i++) g_zob_pass[i] = g_rng();
    g_zob_ready = true;
}

inline U64 state_hash(const Bitboard& s, int8 player) {
    U64 h = 0ULL;
    U64 t = s.p1;
    while (t) {
        int idx = lsb_index(t);
        t &= (t - 1);
        h ^= g_zob[idx][1];
    }
    t = s.p2;
    while (t) {
        int idx = lsb_index(t);
        t &= (t - 1);
        h ^= g_zob[idx][2];
    }
    if (player == 2) h ^= g_zob_side;
    if (s.ply >= 0 && s.ply <= MAX_STEPS) h ^= g_zob_ply[s.ply];
    h ^= g_zob_pass[s.pass_count];
    return h;
}

void tt_clear() {
    memset(g_tt, 0, sizeof(g_tt));
}

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

void killer_clear() {
    memset(g_killer, 0, sizeof(g_killer));
}

inline void killer_update(int depth, const Move& mv) {
    if (depth < 0 || depth >= MAX_DEPTH) return;
    if (g_killer[depth][0] == mv) return;
    g_killer[depth][1] = g_killer[depth][0];
    g_killer[depth][0] = mv;
}

/* Game State */

Bitboard board_to_state(const int board[MAX_M][MAX_N], int step) {
    Bitboard s;
    for (int r = 0; r < 7; r++) {
        for (int c = 0; c < 7; c++) {
            U64 b = bit_at(r, c);
            if (board[r][c] == 1) s.p1 |= b;
            else if (board[r][c] == 2) s.p2 |= b;
        }
    }
    s.ply = step - 1;
    s.pass_count = 0;
    return s;
}

int check_win(const Bitboard& s, int8 player) {
    // Check winning and terminal conditions. +WIN_SCORE for P1 win, -WIN_SCORE for P2 win, 0 for draw, INF_SCORE for non-terminal.
    int p1 = popcnt(s.p1);
    int p2 = popcnt(s.p2);
    if (p1 == 0) return (player == 2 ? WIN_SCORE : -WIN_SCORE);
    if (p2 == 0) return (player == 1 ? WIN_SCORE : -WIN_SCORE);

    U64 occ = s.p1 | s.p2;
    if (occ == g_valid_mask || s.ply >= MAX_STEPS || s.pass_count >= 2) {
        if (p1 > p2) return (player == 1 ? WIN_SCORE : -WIN_SCORE);
        if (p2 > p1) return (player == 2 ? WIN_SCORE : -WIN_SCORE);
        return 0;
    }
    return INF_SCORE;
}

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

void order_moves(vector<Move>& moves, int depth, const Move* tt_best) {
    // Priority: tt_best > killer > others
    for (Move& mv : moves) {
        if (tt_best && mv == *tt_best) mv.pri += 2000000;
        if (depth < MAX_DEPTH && mv == g_killer[depth][0]) mv.pri += 500000;
        else if (depth < MAX_DEPTH && mv == g_killer[depth][1]) mv.pri += 300000;
    }
    sort(moves.begin(), moves.end());
}

Bitboard apply_move(const Bitboard& s, int8 player, const Move& mv) {
    Bitboard ns = s;
    U64 src_b = bit_at(mv.sr, mv.sc);
    U64 dst_b = bit_at(mv.dr, mv.dc);

    if (player == 1) {
        if (!mv.is_clone) ns.p1 &= ~src_b;
        ns.p1 |= dst_b;
    } else {
        if (!mv.is_clone) ns.p2 &= ~src_b;
        ns.p2 |= dst_b;
    }

    int didx = mv.dr * 8 + mv.dc;
    U64 flip = g_adj_mask[didx] & (player == 1 ? ns.p2 : ns.p1);
    if (player == 1) {
        ns.p2 &= ~flip;
        ns.p1 |= flip;
    } else {
        ns.p1 &= ~flip;
        ns.p2 |= flip;
    }

    ns.ply++;
    ns.pass_count = 0;
    return ns;
}

Bitboard apply_pass(const Bitboard& s) {
    Bitboard ns = s;
    ns.ply++;
    ns.pass_count = min(2, s.pass_count + 1);
    return ns;
}

/* Heuristic
estimate() returns the advantage of P1.
evaluate() will be used externally, and it will handle player side and check winning state before calling estimate().
*/

class Heuristic {
public:
    virtual ~Heuristic() {}
    virtual const char* name() const = 0;
    virtual int estimate(const Bitboard& s) const = 0; // positive means P1 better

    int evaluate(const Bitboard& s, int8 player) const {
        int terminal = check_win(s, player);
        if (terminal != INF_SCORE) return terminal;
        int val = estimate(s);
        return (player == 1 ? val : -val);
    }
};

class MaterialHeuristic : public Heuristic {
public:
    const char* name() const { return "Material"; }
    int estimate(const Bitboard& s) const {
        return (popcnt(s.p1) - popcnt(s.p2)) * 50;
    }
};

class MobilityHeuristic : public Heuristic {
public:
    const char* name() const { return "Mobility"; }
    int estimate(const Bitboard& s) const {
        vector<Move> m1, m2;
        generate_moves(s, 1, m1);
        generate_moves(s, 2, m2);
        return (popcnt(s.p1) - popcnt(s.p2)) * 50 + (m1.size() - m2.size()) * 6;
    }
};

class CenterControlHeuristic : public Heuristic {
public:
    const char* name() const { return "CenterControl"; }
    int estimate(const Bitboard& s) const {
        int score = (popcnt(s.p1) - popcnt(s.p2)) * 50;
        U64 t = s.p1;
        while (t) {
            int idx = lsb_index(t);
            t &= (t - 1);
            int r = idx / 8;
            int c = idx % 8;
            score += 50 - center_dist2(r, c) * 4;
        }
        t = s.p2;
        while (t) {
            int idx = lsb_index(t);
            t &= (t - 1);
            int r = idx / 8;
            int c = idx % 8;
            score -= 50 - center_dist2(r, c) * 4;
        }
        return score;
    }
};

class InfectionPressureHeuristic : public Heuristic {
public:
    const char* name() const { return "InfectionPressure"; }
    int estimate(const Bitboard& s) const {
        int score = (popcnt(s.p1) - popcnt(s.p2)) * 50;

        U64 t = s.p1;
        while (t) {
            int idx = lsb_index(t);
            t &= (t - 1);
            U64 near_enemy = g_adj_mask[idx] & s.p2;
            U64 near_friend = g_adj_mask[idx] & s.p1;
            score += popcnt(near_enemy) * 12;
            score += popcnt(near_friend) * 4;
        }

        t = s.p2;
        while (t) {
            int idx = lsb_index(t);
            t &= (t - 1);
            U64 near_enemy = g_adj_mask[idx] & s.p1;
            U64 near_friend = g_adj_mask[idx] & s.p2;
            score -= popcnt(near_enemy) * 12;
            score -= popcnt(near_friend) * 4;
        }
        return score;
    }
};

class ExpansionHeuristic : public Heuristic {
public:
    const char* name() const { return "Expansion"; }
    int estimate(const Bitboard& s) const {
        int score = (popcnt(s.p1) - popcnt(s.p2)) * 50;
        U64 occ = s.p1 | s.p2;
        U64 empty = g_valid_mask & (~occ);

        U64 t = s.p1;
        while (t) {
            int idx = lsb_index(t);
            t &= (t - 1);
            score += popcnt(g_clone_dst_mask[idx] & empty) * 6;
            score += popcnt(g_jump_dst_mask[idx] & empty) * 3;
        }
        t = s.p2;
        while (t) {
            int idx = lsb_index(t);
            t &= (t - 1);
            score -= popcnt(g_clone_dst_mask[idx] & empty) * 6;
            score -= popcnt(g_jump_dst_mask[idx] & empty) * 3;
        }
        return score;
    }
};

class SafetyHeuristic : public Heuristic {
public:
    const char* name() const { return "Safety"; }
    int estimate(const Bitboard& s) const {
        int score = (popcnt(s.p1) - popcnt(s.p2)) * 50;
        U64 t = s.p1;
        while (t) {
            int idx = lsb_index(t);
            t &= (t - 1);
            int friendly = popcnt(g_adj_mask[idx] & s.p1);
            int enemy = popcnt(g_adj_mask[idx] & s.p2);
            score += friendly * 6 - enemy * 8;
        }
        t = s.p2;
        while (t) {
            int idx = lsb_index(t);
            t &= (t - 1);
            int friendly = popcnt(g_adj_mask[idx] & s.p2);
            int enemy = popcnt(g_adj_mask[idx] & s.p1);
            score -= friendly * 6 - enemy * 8;
        }
        return score;
    }
};

class InfluenceHeuristic : public Heuristic {
public:
    const char* name() const { return "Influence"; }
    int estimate(const Bitboard& s) const {
        int score = (popcnt(s.p1) - popcnt(s.p2)) * 50;
        U64 occ = s.p1 | s.p2;
        U64 empty = g_valid_mask & (~occ);
        U64 e = empty;
        while (e) {
            int idx = lsb_index(e);
            e &= (e - 1);
            int p1_near = popcnt(g_adj_mask[idx] & s.p1);
            int p2_near = popcnt(g_adj_mask[idx] & s.p2);
            score += (p1_near - p2_near) * 9;
        }
        return score;
    }
};

class FrontierHeuristic : public Heuristic {
public:
    const char* name() const { return "Frontier"; }
    int estimate(const Bitboard& s) const {
        int p1n = popcnt(s.p1), p2n = popcnt(s.p2);
        int empties = 49 - p1n - p2n;
        int score = (p1n - p2n) * 50;

        U64 occ = s.p1 | s.p2;
        U64 empty = g_valid_mask & (~occ);
        int p1_front = 0, p2_front = 0;

        U64 t = s.p1;
        while (t) {
            int idx = lsb_index(t);
            t &= (t - 1);
            if (g_adj_mask[idx] & empty) p1_front++;
        }
        t = s.p2;
        while (t) {
            int idx = lsb_index(t);
            t &= (t - 1);
            if (g_adj_mask[idx] & empty) p2_front++;
        }

        if (empties > 16) {
            score += (p1_front - p2_front) * 6;
        } else {
            score -= (p1_front - p2_front) * 6;
        }
        return score;
    }
};

class HybridHeuristic : public Heuristic {
public:
    const char* name() const { return "Hybrid"; }
    int estimate(const Bitboard& s) const {
        int score = (popcnt(s.p1) - popcnt(s.p2)) * 50;
        U64 occ = s.p1 | s.p2;
        U64 empty = g_valid_mask & (~occ);

        U64 t = s.p1;
        while (t) {
            int idx = lsb_index(t);
            t &= (t - 1);
            int r = idx / 8, c = idx % 8;
            score += 40 - center_dist2(r, c) * 3;
            score += popcnt(g_adj_mask[idx] & s.p2) * 10;
            score += popcnt(g_clone_dst_mask[idx] & empty) * 4;
        }

        t = s.p2;
        while (t) {
            int idx = lsb_index(t);
            t &= (t - 1);
            int r = idx / 8, c = idx % 8;
            score -= 40 - center_dist2(r, c) * 3;
            score -= popcnt(g_adj_mask[idx] & s.p1) * 10;
            score -= popcnt(g_clone_dst_mask[idx] & empty) * 4;
        }
        return score;
    }
};

class PositionWeightHeuristic : public Heuristic {
private:
    static const int pos_weight[7][7];
public:
    const char* name() const { return "PositionWeight"; }

    int estimate(const Bitboard& s) const {
        int score = (popcnt(s.p1) - popcnt(s.p2)) * 50;
        U64 p1 = s.p1;
        U64 p2 = s.p2;
        while (p1) {
            int idx = lsb_index(p1);
            p1 &= p1 - 1;
            int r = idx / 8;
            int c = idx % 8;
            score += pos_weight[r][c];
        }
        while (p2) {
            int idx = lsb_index(p2);
            p2 &= p2 - 1;
            int r = idx / 8;
            int c = idx % 8;
            score -= pos_weight[r][c];
        }
        return score;
    }
};

const int PositionWeightHeuristic::pos_weight[7][7] = {
    {90, -20, 10, 5, 10, -20, 90},
    {-20, -50, -5, 0, -5, -50, -20},
    {10, -5, 20, 15, 20, -5, 10},
    {5, 0, 15, 30, 15, 0, 5},
    {10, -5, 20, 15, 20, -5, 10},
    {-20, -50, -5, 0, -5, -50, -20},
    {90, -20, 10, 5, 10, -20, 90}
};

class PotentialConversionHeuristic : public Heuristic {
public:
    const char* name() const { return "PotentialConversion"; }

    int estimate(const Bitboard& s) const {
        int score = (popcnt(s.p1) - popcnt(s.p2)) * 50;
        U64 p1 = s.p1;
        U64 p2 = s.p2;
        while (p1) {
            int idx = lsb_index(p1);
            p1 &= p1 - 1;
            score += popcnt(g_adj_mask[idx] & s.p2) * 25;
        }
        while (p2) {
            int idx = lsb_index(p2);
            p2 &= p2 - 1;
            score -= popcnt(g_adj_mask[idx] & s.p1) * 25;
        }
        return score;
    }
};

array<vector<pair<int, int>>, 64> g_control_terms;
bool g_control_terms_ready = false;

void init_control_terms() {
    if (g_control_terms_ready) return;
    for (int sr = 0; sr < 7; sr++) {
        for (int sc = 0; sc < 7; sc++) {
            int sidx = sr * 8 + sc;
            g_control_terms[sidx].clear();
            for (int dr = 0; dr < 7; dr++) {
                for (int dc = 0; dc < 7; dc++) {
                    int dist = abs(sr - dr) + abs(sc - dc);
                    int w = 0;
                    if (dist == 1) w = 32;
                    else if (dist == 2) w = 16;
                    if (w != 0) g_control_terms[sidx].push_back({dr * 8 + dc, w});
                }
            }
        }
    }
    g_control_terms_ready = true;
}

class ControlAreaHeuristic : public Heuristic {
public:
    ControlAreaHeuristic() { init_control_terms(); }
    const char* name() const { return "ControlArea"; }

    int estimate(const Bitboard& s) const {
        int score = (popcnt(s.p1) - popcnt(s.p2)) * 50;
        U64 p1 = s.p1;
        U64 p2 = s.p2;
        U64 occ = s.p1 | s.p2;

        while (p1) {
            int idx = lsb_index(p1);
            p1 &= p1 - 1;
            for (const pair<int, int>& term : g_control_terms[idx]) {
                U64 b = 1ULL << term.first;
                if ((occ & b) == 0) score += term.second;
            }
        }
        while (p2) {
            int idx = lsb_index(p2);
            p2 &= p2 - 1;
            for (const pair<int, int>& term : g_control_terms[idx]) {
                U64 b = 1ULL << term.first;
                if ((occ & b) == 0) score -= term.second;
            }
        }
        return score;
    }
};

class AggressionHeuristic : public Heuristic {
public:
    const char* name() const { return "Aggression"; }

    int estimate(const Bitboard& s) const {
        int score = (popcnt(s.p1) - popcnt(s.p2)) * 50;

        U64 p1 = s.p1;
        while (p1) {
            int idx = lsb_index(p1);
            p1 &= p1 - 1;
            int r = idx / 8;
            int c = idx % 8;
            int best = 100;
            U64 e = s.p2;
            while (e) {
                int eidx = lsb_index(e);
                e &= e - 1;
                int er = eidx / 8;
                int ec = eidx % 8;
                int d = abs(r - er) + abs(c - ec);
                if (d < best) best = d;
            }
            score += (12 - best) * 8;
        }

        U64 p2 = s.p2;
        while (p2) {
            int idx = lsb_index(p2);
            p2 &= p2 - 1;
            int r = idx / 8;
            int c = idx % 8;
            int best = 100;
            U64 e = s.p1;
            while (e) {
                int eidx = lsb_index(e);
                e &= e - 1;
                int er = eidx / 8;
                int ec = eidx % 8;
                int d = abs(r - er) + abs(c - ec);
                if (d < best) best = d;
            }
            score -= (12 - best) * 8;
        }
        return score;
    }
};

class AdaptiveHeuristic : public Heuristic {
public:
    const char* name() const { return "Adaptive"; }

    int estimate(const Bitboard& s) const {
        int p1n = popcnt(s.p1);
        int p2n = popcnt(s.p2);
        int occupied = p1n + p2n;
        double phase = (double)occupied / 49.0;

        U64 occ = s.p1 | s.p2;
        U64 empty = g_valid_mask & (~occ);

        int center = 0;
        int expansion = 0;
        int pressure = 0;

        U64 t = s.p1;
        while (t) {
            int idx = lsb_index(t);
            t &= t - 1;
            int r = idx / 8;
            int c = idx % 8;
            center += 30 - center_dist2(r, c) * 2;
            expansion += popcnt(g_clone_dst_mask[idx] & empty) * 4 + popcnt(g_jump_dst_mask[idx] & empty) * 2;
            pressure += popcnt(g_adj_mask[idx] & s.p2) * 8;
        }

        t = s.p2;
        while (t) {
            int idx = lsb_index(t);
            t &= t - 1;
            int r = idx / 8;
            int c = idx % 8;
            center -= 30 - center_dist2(r, c) * 2;
            expansion -= popcnt(g_clone_dst_mask[idx] & empty) * 4 + popcnt(g_jump_dst_mask[idx] & empty) * 2;
            pressure -= popcnt(g_adj_mask[idx] & s.p1) * 8;
        }

        int score = (p1n - p2n) * 50;
        int w_center = (28 - 10 * phase);
        int w_exp = (34 - 24 * phase);
        int w_pressure = (12 + 20 * phase);
        score += center * w_center / 20;
        score += expansion * w_exp / 20;
        score += pressure * w_pressure / 20;
        return score;
    }
};

class CenterExpansionHeuristic : public Heuristic {
public:
    const char* name() const { return "CenterExpansion"; }
    int estimate(const Bitboard& s) const {
        int score = (popcnt(s.p1) - popcnt(s.p2)) * 50;
        U64 occ = s.p1 | s.p2;
        U64 empty = g_valid_mask & (~occ);

        U64 t = s.p1;
        while (t) {
            int idx = lsb_index(t);
            t &= t - 1;
            int r = idx / 8;
            int c = idx % 8;
            score += 36 - center_dist2(r, c) * 3;
            score += popcnt(g_clone_dst_mask[idx] & empty) * 5;
            score += popcnt(g_jump_dst_mask[idx] & empty) * 2;
        }
        t = s.p2;
        while (t) {
            int idx = lsb_index(t);
            t &= t - 1;
            int r = idx / 8;
            int c = idx % 8;
            score -= 36 - center_dist2(r, c) * 3;
            score -= popcnt(g_clone_dst_mask[idx] & empty) * 5;
            score -= popcnt(g_jump_dst_mask[idx] & empty) * 2;
        }
        return score;
    }
};

class CenterPressurePCHeuristic : public Heuristic {
public:
    const char* name() const { return "CenterPressurePC"; }
    int estimate(const Bitboard& s) const {
        int score = (popcnt(s.p1) - popcnt(s.p2)) * 50;
        U64 t = s.p1;
        while (t) {
            int idx = lsb_index(t);
            t &= t - 1;
            int r = idx / 8;
            int c = idx % 8;
            score += 34 - center_dist2(r, c) * 2;
            score += popcnt(g_adj_mask[idx] & s.p2) * 11;
            score += popcnt(g_adj_mask[idx] & s.p2) * 7;
        }
        t = s.p2;
        while (t) {
            int idx = lsb_index(t);
            t &= t - 1;
            int r = idx / 8;
            int c = idx % 8;
            score -= 34 - center_dist2(r, c) * 2;
            score -= popcnt(g_adj_mask[idx] & s.p1) * 11;
            score -= popcnt(g_adj_mask[idx] & s.p1) * 7;
        }
        return score;
    }
};

class PressureExpansionHeuristic : public Heuristic {
public:
    const char* name() const { return "PressureExpansion"; }
    int estimate(const Bitboard& s) const {
        int score = (popcnt(s.p1) - popcnt(s.p2)) * 50;
        U64 occ = s.p1 | s.p2;
        U64 empty = g_valid_mask & (~occ);

        U64 t = s.p1;
        while (t) {
            int idx = lsb_index(t);
            t &= t - 1;
            score += popcnt(g_adj_mask[idx] & s.p2) * 10;
            score += popcnt(g_clone_dst_mask[idx] & empty) * 4;
            score += popcnt(g_jump_dst_mask[idx] & empty) * 2;
        }
        t = s.p2;
        while (t) {
            int idx = lsb_index(t);
            t &= t - 1;
            score -= popcnt(g_adj_mask[idx] & s.p1) * 10;
            score -= popcnt(g_clone_dst_mask[idx] & empty) * 4;
            score -= popcnt(g_jump_dst_mask[idx] & empty) * 2;
        }
        return score;
    }
};

/* Add (material_w_ * Material difference) to a base heuristic */

class MaterialPlusHeuristic : public Heuristic {
private:
    const Heuristic* h_base;
    int material_w_;
    string name_;

public:
    MaterialPlusHeuristic(const Heuristic* base, int material_w)
        : h_base(base), material_w_(material_w) {
        name_ = string(h_base->name()) + "+Mat" + to_string(material_w_);
    }

    const char* name() const { return name_.c_str(); }

    int estimate(const Bitboard& s) const {
        int mat = popcnt(s.p1) - popcnt(s.p2);
        return h_base->estimate(s) + material_w_ * mat;
    }
};

class APlusBHeuristic : public Heuristic {
private:
    const Heuristic* a_;
    const Heuristic* b_;
    int w_b_;
    int scale_;
    string name_;

public:
    APlusBHeuristic(const Heuristic* a, const Heuristic* b, int w_b, int scale = 100)
        : a_(a), b_(b), w_b_(w_b), scale_(scale) {
        name_ = string(a_->name()) + "+" + b_->name() + "x" + to_string(w_b_);
    }

    const char* name() const { return name_.c_str(); }

    int estimate(const Bitboard& s) const {
        return a_->estimate(s) + (b_->estimate(s) * w_b_) / scale_;
    }
};

/* Search framework */

int ab_negamax(const Bitboard& s, U64 hash, int depth, int max_depth, int8 player,
               const Heuristic* h, int alpha, int beta) {
    static int node_count = 0;
    if (g_time_limit_ms > 0.0 && ((++node_count & 8191) == 0)) { // Check time every 8192 nodes
        double elapsed = chrono::duration<double, milli>(
            chrono::steady_clock::now() - g_start_time
        ).count();
        if (elapsed > g_time_limit_ms) g_time_out = true;
    }
    if (g_time_out) return 0; // Do not trust time-out score, just return 0 to stop search immediately.

    int term = check_win(s, player);
    if (term != INF_SCORE) {
        if (term > 0) return term - depth;
        if (term < 0) return term + depth;
        return 0;
    }

    if (depth == max_depth) {
        return h->evaluate(s, player);
    }

    int remain = max_depth - depth;
    TTEntry* entry = tt_probe(hash);
    Move tt_best;
    bool has_tt_best = false;
    if (entry && entry->valid) {
        tt_best = entry->best;
        has_tt_best = true;
        if (entry->depth >= remain) {
            if (entry->flag == TT_EXACT) return entry->score;
            if (entry->flag == TT_LOWER) alpha = max(alpha, entry->score);
            else if (entry->flag == TT_UPPER) beta = min(beta, entry->score);
            if (alpha >= beta) return entry->score;
        }
    }

    vector<Move> moves;
    generate_moves(s, player, moves);

    if (moves.empty()) {
        Bitboard ns = apply_pass(s);
        U64 nh = state_hash(ns, (3 - player));
        return -ab_negamax(ns, nh, depth + 1, max_depth, (3 - player), h, -beta, -alpha);
    }

    order_moves(moves, depth, has_tt_best ? &tt_best : nullptr);

    int best_score = -INF_SCORE;
    Move best_move = moves[0];
    int alpha0 = alpha;
    int flag = TT_UPPER;

    // TT_EXACT means the move is the best move for sure; TT_LOWER means the move is at least this good; TT_UPPER means the move is at most this good.
    for (const Move& mv : moves) {
        Bitboard ns = apply_move(s, player, mv);
        U64 nh = state_hash(ns, (3 - player));
        int score = -ab_negamax(ns, nh, depth + 1, max_depth, (3 - player), h, -beta, -alpha);
        if (g_time_out) return 0;

        if (score > best_score) {
            best_score = score;
            best_move = mv;
        }
        if (score > alpha) {
            alpha = score;
            flag = TT_EXACT;
        }
        if (alpha >= beta) { // beta-cutoff
            flag = TT_LOWER;
            killer_update(depth, mv);
            break;
        }
    }

    if (best_score <= alpha0) flag = TT_UPPER;
    tt_store(hash, remain, flag, best_score, best_move);
    return best_score;
}

Move ab_solver(const Bitboard& s, int8 player, const Heuristic* h, int max_depth) {
    tt_clear();
    killer_clear();
    g_time_out = false;
    g_time_limit_ms = 0.0;

    vector<Move> moves;
    generate_moves(s, player, moves);
    if (moves.empty()) return Move();

    Move best = moves[0];
    int alpha = -INF_SCORE;
    int beta = INF_SCORE;
    U64 hash = state_hash(s, player);

    for (const Move& mv : moves) {
        Bitboard ns = apply_move(s, player, mv);
        U64 nh = state_hash(ns, (3 - player));
        int score = -ab_negamax(ns, nh, 1, max_depth, (3 - player), h, -beta, -alpha);
        if (score > alpha) {
            alpha = score;
            best = mv;
        }
    }
    return best;
}

Move iterative_deepening_solver(const Bitboard& s, int8 player, const Heuristic* h,
                                int max_depth, double time_limit_ms) {
    tt_clear();
    killer_clear();

    vector<Move> root_moves;
    generate_moves(s, player, root_moves);
    if (root_moves.empty()) return Move();
    if (root_moves.size() == 1) return root_moves[0];

    g_time_out = false;
    g_start_time = chrono::steady_clock::now();
    g_time_limit_ms = time_limit_ms;

    Move best = root_moves[0];
    U64 root_hash = state_hash(s, player);
    int n_moves = root_moves.size();

    for (int depth = 1; depth <= max_depth; depth++) {
        if (g_time_out) break;

        for (int i = 0; i < n_moves; i++) {
            if (root_moves[i] == best) {
                swap(root_moves[0], root_moves[i]);
                break;
            }
        }

        int alpha = -INF_SCORE;
        int beta = INF_SCORE;
        int best_score_depth = -INF_SCORE;
        Move best_depth = best;
        bool complete = true;

        for (Move& mv : root_moves) {
            Bitboard ns = apply_move(s, player, mv);
            U64 nh = state_hash(ns, (3 - player));
            int score = -ab_negamax(ns, nh, 1, depth, (3 - player), h, -beta, -max(alpha, best_score_depth));

            if (g_time_out) {
                complete = false;
                break;
            }
            if (score > best_score_depth) {
                best_score_depth = score;
                best_depth = mv;
                alpha = max(alpha, score);
            }
        }

        if (complete) {
            best = best_depth;
            if (best_score_depth > WIN_SCORE / 2) break;
        } else {
            break;
        }
    }

    return best;
}

struct MCTSNode {
    Bitboard s;
    int8 player;
    Move from_parent;
    MCTSNode* parent;
    vector<MCTSNode*> children;
    vector<Move> untried;
    int visits;
    double win_sum;

    MCTSNode(const Bitboard& _s, int8 _player, const Move& mv, MCTSNode* p)
        : s(_s), player(_player), from_parent(mv), parent(p), visits(0), win_sum(0.0) {
        generate_moves(s, player, untried);
    }

    ~MCTSNode() {
        for (MCTSNode* c : children) delete c;
    }

    bool terminal() const {
        return check_win(s, player) != INF_SCORE;
    }
};

int winner_from_state(const Bitboard& s) {
    int p1 = popcnt(s.p1);
    int p2 = popcnt(s.p2);
    if (p1 == 0) return 2;
    if (p2 == 0) return 1;
    U64 occ = s.p1 | s.p2;
    if (occ == g_valid_mask || s.ply >= MAX_STEPS || s.pass_count >= 2) {
        if (p1 > p2) return 1;
        if (p2 > p1) return 2;
        return 0;
    }
    return -1;
}

Move choose_playout_move(const Bitboard& s, int8 player, const Heuristic* h, double eps) {
    vector<Move> moves;
    generate_moves(s, player, moves);
    if (moves.empty()) return Move();

    // if no heuristic, or with probability eps, choose a random move for more exploration
    if (h == nullptr || (double)(g_rng() % 10000000) / 10000000.0 < eps) {
        return moves[g_rng() % moves.size()];
    }

    // Otherwise, choose the best move according to heuristic
    int best = -INF_SCORE;
    Move best_mv = moves[0];
    for (const Move& mv : moves) {
        Bitboard ns = apply_move(s, player, mv);
        int v = h->evaluate(ns, player);
        if (v > best) {
            best = v;
            best_mv = mv;
        }
    }
    return best_mv;
}

int mcts_playout(Bitboard s, int8 player, const Heuristic* h, int root_player, double eps = 0.65) {
    for (int t = 0; t < 220; t++) {
        int w = winner_from_state(s);
        if (w != -1) return w;

        vector<Move> moves;
        generate_moves(s, player, moves);
        if (moves.empty()) {
            s = apply_pass(s);
            player = (3 - player);
            continue;
        }

        Move mv = choose_playout_move(s, player, h, eps);
        s = apply_move(s, player, mv);
        player = (3 - player);
    }

    int p1 = popcnt(s.p1), p2 = popcnt(s.p2);
    if (p1 > p2) return 1;
    if (p2 > p1) return 2;
    return root_player;
}

Move mcts_solver(const Bitboard& root_state, int8 root_player, const Heuristic* h,
                 int max_iters, double time_limit_ms) {
    MCTSNode* root = new MCTSNode(root_state, root_player, Move(), nullptr);
    if (root->untried.empty()) {
        delete root;
        return Move();
    }

    auto t0 = chrono::steady_clock::now();
    const double K = 1.41421356237;

    for (int iter = 0; iter < max_iters; iter++) {
        if ((iter & 127) == 0 && time_limit_ms > 0.0) {
            double elapsed = chrono::duration<double, milli>(chrono::steady_clock::now() - t0).count();
            if (elapsed > time_limit_ms) break;
        }

        MCTSNode* node = root;
        
        // Selection
        // UCT: u_i = w_i / n_i + K * sqrt(log(N) / n_i)
        while (node->untried.empty() && !node->children.empty() && !node->terminal()) {
            MCTSNode* best = nullptr;
            double best_uct = -1e10;
            double log_vis = log((double)node->visits);
            for (MCTSNode* ch : node->children) {
                double exploit = (double)ch->win_sum / (ch->visits + 1e-9);
                double explore = K * sqrt(log_vis / (ch->visits + 1e-9));
                double uct = exploit + explore;
                if (uct > best_uct) {
                    best_uct = uct;
                    best = ch;
                }
            }
            node = best;
        }

        // Expansion
        if (!node->terminal() && !node->untried.empty()) {
            int pick = (g_rng() % node->untried.size());
            Move mv = node->untried[pick];
            node->untried[pick] = node->untried.back();
            node->untried.pop_back();

            Bitboard ns = apply_move(node->s, node->player, mv);
            MCTSNode* child = new MCTSNode(ns, (3 - node->player), mv, node);
            node->children.push_back(child);
            node = child;
        }

        // Simulation
        int winner = mcts_playout(node->s, node->player, h, root_player);
        double result = 0.5;
        if (winner == root_player) result = 1.0;
        else if (winner == 3 - root_player) result = 0.0;

        // Backpropagation
        while (node) {
            node->visits++;
            node->win_sum += result;
            node = node->parent;
        }
    }

    MCTSNode* best = nullptr;
    int best_visits = -1;
    for (MCTSNode* ch : root->children) {
        if (ch->visits > best_visits) {
            best_visits = ch->visits;
            best = ch;
        }
    }

    Move ans = (best ? best->from_parent : root->untried[0]);
    delete root; // will recursively delete all nodes
    return ans;
}

#ifndef LOCAL

void play_games(int step) {
    static bool initialized = false;
    if (!initialized) {
        init_masks();
        init_zobrist();
        initialized = true;
    }

    Board board;
    read_ckbd(step - 1, board);
    int8 player = (step & 1) ? 1 : 2;
    Bitboard s = board_to_state(board, step);

    static ExpansionHeuristic h_base;
    static MaterialPlusHeuristic h_best(&h_base, 40);
    Move best = iterative_deepening_solver(s, player, &h_best, 10, 1500.0);

    save_decision(best.sr, best.sc, best.dr, best.dc);
}

#endif

#ifdef LOCAL

enum SolverType { SOLVER_AB, SOLVER_ID, SOLVER_MCTS };

struct SolverProfile {
    const char* name;
    SolverType type;
    const Heuristic* h;
    int max_depth;
    int max_iters;
    double time_limit_ms;

    Move get_move(const Bitboard& s, int8 player) const {
        if (type == SOLVER_AB) return ab_solver(s, player, h, max_depth);
        if (type == SOLVER_ID) return iterative_deepening_solver(s, player, h, max_depth, time_limit_ms);
        return mcts_solver(s, player, h, max_iters, time_limit_ms);
    }
};

struct MatchResult {
    int winner;
    int ply;
    double p1_avg_ms;
    double p2_avg_ms;
};

struct TournamentResult {
    vector<int> wins;
    vector<int> timeout_wins;
    vector<double> avg_move_time;
};

static string make_unique_tag() {
    long long ms = chrono::duration_cast<chrono::milliseconds>(
        chrono::system_clock::now().time_since_epoch()
    ).count();
    return to_string(ms);
}

Bitboard initial_state() {
    Bitboard s;
    s.p1 = bit_at(0, 0) | bit_at(6, 6);
    s.p2 = bit_at(0, 6) | bit_at(6, 0);
    s.ply = 0;
    s.pass_count = 0;
    return s;
}

MatchResult play_match(const SolverProfile& p1, const SolverProfile& p2, int max_ply = MAX_STEPS) {
    Bitboard s = initial_state();
    double p1_time = 0.0, p2_time = 0.0;
    int p1_moves = 0, p2_moves = 0;

    for (;;) {
        int w = winner_from_state(s);
        if (w != -1) {
            double p1_avg = (p1_moves ? p1_time / p1_moves : 0.0);
            double p2_avg = (p2_moves ? p2_time / p2_moves : 0.0);
            return {w, s.ply, p1_avg, p2_avg};
        }

        int8 player = (s.ply & 1) ? 2 : 1;
        vector<Move> moves;
        generate_moves(s, player, moves);

        if (moves.empty()) {
            s = apply_pass(s);
            continue;
        }

        auto t0 = chrono::steady_clock::now();
        Move mv = (player == 1) ? p1.get_move(s, 1) : p2.get_move(s, 2);
        auto t1 = chrono::steady_clock::now();
        double dt = chrono::duration<double, milli>(t1 - t0).count();

        bool legal = false;
        for (const Move& m : moves) {
            if (m == mv) {
                legal = true;
                break;
            }
        }
        if (!legal) mv = moves[0];

        s = apply_move(s, player, mv);
        if (player == 1) {
            p1_time += dt;
            p1_moves++;
        } else {
            p2_time += dt;
            p2_moves++;
        }

        if (s.ply >= max_ply) {
            int p1n = popcnt(s.p1);
            int p2n = popcnt(s.p2);
            int ww = (p1n > p2n ? 1 : (p2n > p1n ? 2 : 0));
            double p1_avg = (p1_moves ? p1_time / p1_moves : 0.0);
            double p2_avg = (p2_moves ? p2_time / p2_moves : 0.0);
            return {ww, s.ply, p1_avg, p2_avg};
        }
    }
}

TournamentResult run_tournament(
    const vector<SolverProfile>& solvers,
    int trials,
    const string& csv_path,
    FILE* report_fp,
    const char* stage_name
) {
    int n = solvers.size();
    vector<int> wins(n, 0);
    vector<int> timeout_wins(n, 0);
    vector<int> games(n, 0);
    vector<double> total_time(n, 0.0);

    FILE* fp = fopen(csv_path.c_str(), "w");
    fprintf(fp, "Trial,P1,P2,Winner,Ply,P1AvgMS,P2AvgMS\n");

    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            for (int t = 0; t < trials; t++) {
                bool swapped = (t & 1);
                const SolverProfile& A = swapped ? solvers[j] : solvers[i];
                const SolverProfile& B = swapped ? solvers[i] : solvers[j];

                MatchResult res = play_match(A, B);
                int winner = res.winner;
                int winner_global = 0;
                if (!swapped) {
                    if (winner == 1) winner_global = i;
                    else if (winner == 2) winner_global = j;
                } else {
                    if (winner == 1) winner_global = j;
                    else if (winner == 2) winner_global = i;
                }

                if (winner_global >= 0 && winner_global < n) wins[winner_global]++;
                if (res.ply >= MAX_STEPS && winner_global >= 0 && winner_global < n) timeout_wins[winner_global]++;

                games[i]++;
                games[j]++;
                if (!swapped) {
                    total_time[i] += res.p1_avg_ms;
                    total_time[j] += res.p2_avg_ms;
                } else {
                    total_time[j] += res.p1_avg_ms;
                    total_time[i] += res.p2_avg_ms;
                }

                fprintf(fp, "%d,%s,%s,%d,%d,%.3f,%.3f\n",
                        t + 1, A.name, B.name, winner, res.ply, res.p1_avg_ms, res.p2_avg_ms);
            }
            printf("[%s] %s vs %s finished (%d trials)\n", stage_name, solvers[i].name, solvers[j].name, trials);
        }
    }
    fclose(fp);

    vector<double> avg_time(n, 0.0);
    for (int i = 0; i < n; i++) {
        avg_time[i] = (games[i] ? total_time[i] / games[i] : 0.0);
    }

    vector<int> rank(n);
    for (int i = 0; i < n; i++) rank[i] = i;
    sort(rank.begin(), rank.end(), [&](int a, int b) {
        if (wins[a] != wins[b]) return wins[a] > wins[b];
        if (timeout_wins[a] != timeout_wins[b]) return timeout_wins[a] < timeout_wins[b];
        return avg_time[a] < avg_time[b];
    });

    fprintf(report_fp, "## %s\n\n", stage_name);
    fprintf(report_fp, "- CSV: %s\n", csv_path.c_str());
    fprintf(report_fp, "- Format: iterative deepening only, time limit 1000 ms, %d trials per pair (color swapped by trial parity).\n\n", trials);
    fprintf(report_fp, "| Rank | Heuristic | Wins | Timeout Wins | Avg Move Time (ms) |\n");
    fprintf(report_fp, "|---:|---|---:|---:|---:|\n");
    for (int k = 0; k < n; k++) {
        int i = rank[k];
        fprintf(report_fp, "| %d | %s | %d | %d | %.2f |\n", k + 1, solvers[i].name, wins[i], timeout_wins[i], avg_time[i]);
    }
    fprintf(report_fp, "\n");

    return {wins, timeout_wins, avg_time};
}

int main() {
    init_masks();
    init_zobrist();

    string run_tag = make_unique_tag();
    string report_path = string("out/heuristic_iteration_report_") + run_tag + ".md";
    FILE* report_fp = fopen(report_path.c_str(), "w");

    fprintf(report_fp, "# Ataxx Heuristic Iteration Report\n\n");
    fprintf(report_fp, "Run tag: %s\n\n", run_tag.c_str());
    fprintf(report_fp, "All base heuristics in this run use intrinsic material coefficient = 50.\n");
    fprintf(report_fp, "Ranking key: wins desc, timeout wins asc, avg move time asc.\n\n");

    MaterialHeuristic h_mat;
    MobilityHeuristic h_mob;
    ExpansionHeuristic h_exp;
    HybridHeuristic h_hyb;
    PotentialConversionHeuristic h_pconv;

    CenterExpansionHeuristic h_cexp;
    CenterPressurePCHeuristic h_cppc;
    PressureExpansionHeuristic h_pexp;

    MaterialPlusHeuristic h_exp40(&h_exp, 40);
    MaterialPlusHeuristic h_pc40(&h_pconv, 40);
    MaterialPlusHeuristic h_hyb40(&h_hyb, 40);
    MaterialPlusHeuristic h_cexp40(&h_cexp, 40);
    MaterialPlusHeuristic h_cppc40(&h_cppc, 40);
    MaterialPlusHeuristic h_pexp40(&h_pexp, 40);

    const int trials_main = 20;
    const int id_depth = 6;
    const double id_time_ms = 1000.0;

    vector<SolverProfile> stage9 = {
        {"Material", SOLVER_ID, &h_mat, id_depth, 0, id_time_ms},
        {h_exp40.name(), SOLVER_ID, &h_exp40, id_depth, 0, id_time_ms},
        {h_pc40.name(), SOLVER_ID, &h_pc40, id_depth, 0, id_time_ms},
        {h_hyb40.name(), SOLVER_ID, &h_hyb40, id_depth, 0, id_time_ms}
    };
    string csv9 = string("out/stage9_core_compare_") + run_tag + ".csv";
    TournamentResult r9 = run_tournament(stage9, trials_main, csv9, report_fp, "Stage 9 - Exp40 vs PC40 vs Hybrid40 vs Material");

    vector<int> idx9(stage9.size());
    for (int i = 0; i < stage9.size(); i++) idx9[i] = i;
    sort(idx9.begin(), idx9.end(), [&](int a, int b) {
        if (r9.wins[a] != r9.wins[b]) return r9.wins[a] > r9.wins[b];
        if (r9.timeout_wins[a] != r9.timeout_wins[b]) return r9.timeout_wins[a] < r9.timeout_wins[b];
        return r9.avg_move_time[a] < r9.avg_move_time[b];
    });

    vector<SolverProfile> stage10 = {
        {"Material", SOLVER_ID, &h_mat, id_depth, 0, id_time_ms},
        {h_exp40.name(), SOLVER_ID, &h_exp40, id_depth, 0, id_time_ms},
        {h_pc40.name(), SOLVER_ID, &h_pc40, id_depth, 0, id_time_ms},
        {h_hyb40.name(), SOLVER_ID, &h_hyb40, id_depth, 0, id_time_ms},
        {h_cexp40.name(), SOLVER_ID, &h_cexp40, id_depth, 0, id_time_ms},
        {h_cppc40.name(), SOLVER_ID, &h_cppc40, id_depth, 0, id_time_ms},
        {h_pexp40.name(), SOLVER_ID, &h_pexp40, id_depth, 0, id_time_ms}
    };
    string csv10 = string("out/stage10_recombine_mat40_") + run_tag + ".csv";
    TournamentResult r10 = run_tournament(stage10, trials_main, csv10, report_fp, "Stage 10 - Recombine Under Mat40");

    vector<int> idx10(stage10.size());
    for (int i = 0; i < stage10.size(); i++) idx10[i] = i;
    sort(idx10.begin(), idx10.end(), [&](int a, int b) {
        if (r10.wins[a] != r10.wins[b]) return r10.wins[a] > r10.wins[b];
        if (r10.timeout_wins[a] != r10.timeout_wins[b]) return r10.timeout_wins[a] < r10.timeout_wins[b];
        return r10.avg_move_time[a] < r10.avg_move_time[b];
    });

    vector<SolverProfile> stage11 = {
        {"AB5-Material", SOLVER_AB, &h_mat, 5, 0, 0},
        {"AB6-Material", SOLVER_AB, &h_mat, 6, 0, 0},
        {"AB7-Material", SOLVER_AB, &h_mat, 7, 0, 0},
        {"ID-Material", SOLVER_ID, &h_mat, id_depth, 0, id_time_ms}
    };
    string csv11 = string("out/stage11_ab_vs_id_material_") + run_tag + ".csv";
    TournamentResult r11 = run_tournament(stage11, 10, csv11, report_fp, "Stage 11 - AB Material vs ID Material");

    vector<int> idx11(stage11.size());
    for (int i = 0; i < stage11.size(); i++) idx11[i] = i;
    sort(idx11.begin(), idx11.end(), [&](int a, int b) {
        if (r11.wins[a] != r11.wins[b]) return r11.wins[a] > r11.wins[b];
        if (r11.timeout_wins[a] != r11.timeout_wins[b]) return r11.timeout_wins[a] < r11.timeout_wins[b];
        return r11.avg_move_time[a] < r11.avg_move_time[b];
    });

    vector<SolverProfile> stage12 = {
        {"ID-Hybrid", SOLVER_ID, &h_hyb40, id_depth, 0, id_time_ms},
        {"MCTS-Random", SOLVER_MCTS, nullptr, 0, 200000, 1000.0},
        {"MCTS-Hybrid", SOLVER_MCTS, &h_hyb40, 0, 200000, 1000.0},
        {"MCTS-Material", SOLVER_MCTS, &h_mat, 0, 200000, 1000.0}
    };
    string csv12 = string("out/stage12_id_vs_mcts_") + run_tag + ".csv";
    TournamentResult r12 = run_tournament(stage12, 10, csv12, report_fp, "Stage 12 - ID Hybrid vs MCTS Variants");

    vector<int> idx12(stage12.size());
    for (int i = 0; i < stage12.size(); i++) idx12[i] = i;
    sort(idx12.begin(), idx12.end(), [&](int a, int b) {
        if (r12.wins[a] != r12.wins[b]) return r12.wins[a] > r12.wins[b];
        if (r12.timeout_wins[a] != r12.timeout_wins[b]) return r12.timeout_wins[a] < r12.timeout_wins[b];
        return r12.avg_move_time[a] < r12.avg_move_time[b];
    });

    fprintf(report_fp, "## Selection Summary\n\n");
    if (!idx10.empty()) {
        int best = idx10[0];
        fprintf(report_fp, "Best heuristic after recombination: **%s** (wins=%d, timeout wins=%d, avg move time=%.2f ms).\n\n",
                stage10[best].name, r10.wins[best], r10.timeout_wins[best], r10.avg_move_time[best]);
    }
    fprintf(report_fp, "Round 9 compares Exp+Mat40, PC+Mat40, Hybrid+Mat40 and Material with 20 trials per pair.\n");
    fprintf(report_fp, "Round 10 explores recombinations under Mat40.\n");
    fprintf(report_fp, "Round 11 and Round 12 provide AB/ID/MCTS comparison for report usage.\n\n");

    fclose(report_fp);

    printf("Autopilot run completed.\n");
    printf("Report: %s\n", report_path.c_str());
    printf("CSV 9: %s\n", csv9.c_str());
    printf("CSV 10: %s\n", csv10.c_str());
    printf("CSV 11: %s\n", csv11.c_str());
    printf("CSV 12: %s\n", csv12.c_str());

    return 0;
}

#endif
