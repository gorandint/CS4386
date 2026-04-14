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
#include <set>
#include <tuple>
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

    Bitboard() : p1(0ULL), p2(0ULL), ply(0) {}
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
bool g_zob_ready = false;

/* Search timeout */

bool g_time_out = false;
chrono::steady_clock::time_point g_start_time;
double g_time_limit_ms = 0.0;
int g_clone_base_pri = 60;
int g_capture_weight = 30;
const int g_jump_base_pri = 100;

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
    g_zob_ready = true;
}

inline U64 get_hash(const Bitboard& s, int8 player) {
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
    return h;
}

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
    return s;
}

int check_win(const Bitboard& s, int8 player) {
    // Check winning and terminal conditions. +WIN_SCORE for P1 win, -WIN_SCORE for P2 win, 0 for draw, INF_SCORE for non-terminal.
    int p1 = popcnt(s.p1);
    int p2 = popcnt(s.p2);
    if (p1 == 0) return (player == 2 ? WIN_SCORE : -WIN_SCORE);
    if (p2 == 0) return (player == 1 ? WIN_SCORE : -WIN_SCORE);

    U64 occ = s.p1 | s.p2;
    if (occ == g_valid_mask || s.ply >= MAX_STEPS) {
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
            mv.pri = g_clone_base_pri + cap * g_capture_weight - center_dist2(dr, dc);
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
            mv.pri = g_jump_base_pri + cap * g_capture_weight - center_dist2(dr, dc);
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
    return ns;
}

Bitboard apply_pass(const Bitboard& s) {
    Bitboard ns = s;
    ns.ply++;
    return ns;
}

/* Heuristic
estimate() returns the advantage of P1.
evaluate() will be used externally, and it will handle player side and check winning state before calling estimate().
*/

class Heuristic {
protected:
    int w_mat_;

    int mat_score(const Bitboard& s) const {
        return (popcnt(s.p1) - popcnt(s.p2)) * w_mat_;
    }

public:
    Heuristic(int w_mat = 0) : w_mat_(w_mat) {}
    virtual ~Heuristic() {}
    virtual string get_name() const = 0;
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
    MaterialHeuristic(int w_mat = 0) : Heuristic(w_mat) {}
    string get_name() const { return "Material"; }
    int estimate(const Bitboard& s) const {
        return mat_score(s);
    }
};

class MobilityHeuristic : public Heuristic {
public:
    MobilityHeuristic(int w_mat = 0) : Heuristic(w_mat) {}
    string get_name() const { return "Mobility"; }
    int estimate(const Bitboard& s) const {
        vector<Move> m1, m2;
        generate_moves(s, 1, m1);
        generate_moves(s, 2, m2);
        return mat_score(s) + (m1.size() - m2.size()) * 6;
    }
};

class CenterControlHeuristic : public Heuristic {
public:
    CenterControlHeuristic(int w_mat = 0) : Heuristic(w_mat) {}
    string get_name() const { return "CenterControl"; }
    int estimate(const Bitboard& s) const {
        int score = mat_score(s);
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
    InfectionPressureHeuristic(int w_mat = 0) : Heuristic(w_mat) {}
    string get_name() const { return "InfectionPressure"; }
    int estimate(const Bitboard& s) const {
        int score = mat_score(s);

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
    ExpansionHeuristic(int w_mat = 0) : Heuristic(w_mat) {}
    string get_name() const { return "Expansion"; }
    int estimate(const Bitboard& s) const {
        int score = mat_score(s);
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
    SafetyHeuristic(int w_mat = 0) : Heuristic(w_mat) {}
    string get_name() const { return "Safety"; }
    int estimate(const Bitboard& s) const {
        int score = mat_score(s);
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
    InfluenceHeuristic(int w_mat = 0) : Heuristic(w_mat) {}
    string get_name() const { return "Influence"; }
    int estimate(const Bitboard& s) const {
        int score = mat_score(s);
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
    FrontierHeuristic(int w_mat = 0) : Heuristic(w_mat) {}
    string get_name() const { return "Frontier"; }
    int estimate(const Bitboard& s) const {
        int p1n = popcnt(s.p1), p2n = popcnt(s.p2);
        int empties = 49 - p1n - p2n;
        int score = (p1n - p2n) * w_mat_;

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
    HybridHeuristic(int w_mat = 0) : Heuristic(w_mat) {}
    string get_name() const { return "Hybrid"; }
    int estimate(const Bitboard& s) const {
        int score = mat_score(s);
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
    PositionWeightHeuristic(int w_mat = 0) : Heuristic(w_mat) {}
    string get_name() const { return "PositionWeight"; }

    int estimate(const Bitboard& s) const {
        int score = mat_score(s);
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
    PotentialConversionHeuristic(int w_mat = 0) : Heuristic(w_mat) {}
    string get_name() const { return "PotentialConversion"; }

    int estimate(const Bitboard& s) const {
        int score = mat_score(s);
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
    ControlAreaHeuristic(int w_mat = 0) : Heuristic(w_mat) { init_control_terms(); }
    string get_name() const { return "ControlArea"; }

    int estimate(const Bitboard& s) const {
        int score = mat_score(s);
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
    AggressionHeuristic(int w_mat = 0) : Heuristic(w_mat) {}
    string get_name() const { return "Aggression"; }

    int estimate(const Bitboard& s) const {
        int score = mat_score(s);

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
    AdaptiveHeuristic(int w_mat = 0) : Heuristic(w_mat) {}
    string get_name() const { return "Adaptive"; }

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

        int score = (p1n - p2n) * w_mat_;
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
    CenterExpansionHeuristic(int w_mat = 0) : Heuristic(w_mat) {}
    string get_name() const { return "CenterExpansion"; }
    int estimate(const Bitboard& s) const {
        int score = mat_score(s);
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
    CenterPressurePCHeuristic(int w_mat = 0) : Heuristic(w_mat) {}
    string get_name() const { return "CenterPressurePC"; }
    int estimate(const Bitboard& s) const {
        int score = mat_score(s);
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
    PressureExpansionHeuristic(int w_mat = 0) : Heuristic(w_mat) {}
    string get_name() const { return "PressureExpansion"; }
    int estimate(const Bitboard& s) const {
        int score = mat_score(s);
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
        : Heuristic(0), h_base(base), material_w_(material_w) {
        name_ = h_base->get_name() + "+Mat" + to_string(material_w_);
    }

    string get_name() const { return name_; }

    int estimate(const Bitboard& s) const {
        int mat = popcnt(s.p1) - popcnt(s.p2);
        return h_base->estimate(s) + material_w_ * mat;
    }
};

class APlusBHeuristic : public Heuristic {
private:
    const Heuristic* a_;
    const Heuristic* b_;
    string name_;

public:
    APlusBHeuristic(const Heuristic* a, const Heuristic* b, int w_mat=0)
        : Heuristic(w_mat), a_(a), b_(b) {
        name_ = a_->get_name() + "+" + b_->get_name() + "+Mat" + to_string(w_mat_);
    }

    string get_name() const { return name_; }

    int estimate(const Bitboard& s) const {
        return a_->estimate(s) + b_->estimate(s) + mat_score(s);
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
        U64 nh = incremental_hash(hash, s, ns);
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
        U64 nh = incremental_hash(hash, s, ns);
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
    U64 hash = get_hash(s, player);

    for (const Move& mv : moves) {
        Bitboard ns = apply_move(s, player, mv);
        U64 nh = incremental_hash(hash, s, ns);
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
    U64 root_hash = get_hash(s, player);
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
            U64 nh = incremental_hash(root_hash, s, ns);
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
    if (occ == g_valid_mask || s.ply >= MAX_STEPS) {
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

    static ExpansionHeuristic h_base(0);
    static MaterialPlusHeuristic h_best(&h_base, 40);
    // static InfluenceHeuristic h1;
    // static PotentialConversionHeuristic h2;
    // static ExpansionHeuristic h2;
    // static APlusBHeuristic h_best(&h1, &h2, 0);
    Move best = iterative_deepening_solver(s, player, &h_best, 10000, 1500.0);

    save_decision(best.sr, best.sc, best.dr, best.dc);
}

#endif

#ifdef LOCAL

enum SolverType { SOLVER_AB, SOLVER_ID, SOLVER_MCTS };

struct SolverProfile {
    string name;
    SolverType type;
    const Heuristic* h;
    int max_depth;
    int max_iters;
    double time_limit_ms;
    int clone_base_pri;
    int capture_weight;

    Move get_move(const Bitboard& s, int8 player) const {
        g_clone_base_pri = clone_base_pri;
        g_capture_weight = capture_weight;
        if (type == SOLVER_AB) return ab_solver(s, player, h, max_depth);
        if (type == SOLVER_ID) return iterative_deepening_solver(s, player, h, MAX_DEPTH, time_limit_ms);
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
    vector<vector<int>> done(n, vector<int>(n, 0));

    auto solver_index = [&](const string& nm) {
        for (int i = 0; i < n; i++) {
            if (solvers[i].name == nm) return i;
        }
        return -1;
    };

    bool has_existing = false;
    {
        FILE* rp = fopen(csv_path.c_str(), "r");
        if (rp) {
            has_existing = true;
            char line[1024];
            fgets(line, sizeof(line), rp); // skip header
            while (fgets(line, sizeof(line), rp)) {
                int trial = 0, winner = 0, ply = 0;
                char p1_name[256], p2_name[256];
                double p1_avg = 0.0, p2_avg = 0.0;
                if (sscanf(line, "%d,%255[^,],%255[^,],%d,%d,%lf,%lf",
                           &trial, p1_name, p2_name, &winner, &ply, &p1_avg, &p2_avg) != 7) {
                    continue;
                }

                int a = solver_index(string(p1_name));
                int b = solver_index(string(p2_name));
                if (a < 0 || b < 0 || a == b) continue;

                int i = min(a, b);
                int j = max(a, b);
                bool swapped = (a == j && b == i);

                int winner_global = 0;
                if (!swapped) {
                    if (winner == 1) winner_global = i;
                    else if (winner == 2) winner_global = j;
                } else {
                    if (winner == 1) winner_global = j;
                    else if (winner == 2) winner_global = i;
                }

                if (winner_global >= 0 && winner_global < n) wins[winner_global]++;
                if (ply >= MAX_STEPS && winner_global >= 0 && winner_global < n) timeout_wins[winner_global]++;

                games[i]++;
                games[j]++;
                if (!swapped) {
                    total_time[i] += p1_avg;
                    total_time[j] += p2_avg;
                } else {
                    total_time[j] += p1_avg;
                    total_time[i] += p2_avg;
                }

                if (done[i][j] < trials) done[i][j]++;
            }
            fclose(rp);
        }
    }

    FILE* fp = nullptr;
    if (has_existing) {
        fp = fopen(csv_path.c_str(), "a");
    } else {
        fp = fopen(csv_path.c_str(), "w");
        fprintf(fp, "Trial,P1,P2,Winner,Ply,P1AvgMS,P2AvgMS\n");
    }

    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            for (int t = done[i][j]; t < trials; t++) {
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
                        t + 1, A.name.c_str(), B.name.c_str(), winner, res.ply, res.p1_avg_ms, res.p2_avg_ms);
            }
            printf("[%s] %s vs %s finished (%d/%d trials)\n", stage_name, solvers[i].name.c_str(), solvers[j].name.c_str(), trials, trials);
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
        fprintf(report_fp, "| %d | %s | %d | %d | %.2f |\n", k + 1, solvers[i].name.c_str(), wins[i], timeout_wins[i], avg_time[i]);
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
    fprintf(report_fp, "Stage 24: random search on MoveOrdering and Material weight.\n");
    fprintf(report_fp, "Ranking key: wins desc, timeout wins asc, avg move time asc.\n\n");

    const int trials_main = 6;
    const double id_time_ms = 1000.0;

    InfluenceHeuristic h_infl0(0);
    ExpansionHeuristic h_exp0(0);
    APlusBHeuristic h_infl_exp_m60(&h_infl0, &h_exp0, 60);

    InfluenceHeuristic h_infl0_b(0);
    ExpansionHeuristic h_exp0_b(0);
    APlusBHeuristic h_infl_exp_m60_b(&h_infl0_b, &h_exp0_b, 60);

    vector<ExpansionHeuristic*> random_exp_heuristics;
    vector<SolverProfile> stage24;

    set<tuple<int, int, int>> exp_params;
    exp_params.insert(make_tuple(60, 30, 70)); // known strong baseline expansion

    uniform_int_distribution<int> dist_cl(4, 32);   // 40..320 (step 10)
    uniform_int_distribution<int> dist_ca(2, 10);   // 10..50 (step 5)
    uniform_int_distribution<int> dist_m(2, 10);    // 20..100 (step 10)

    while ((int)exp_params.size() < 20) {
        int cl = dist_cl(g_rng) * 10;
        int ca = dist_ca(g_rng) * 5;
        int mw = dist_m(g_rng) * 10;
        exp_params.insert(make_tuple(cl, ca, mw));
    }

    for (const auto& p : exp_params) {
        int cl = get<0>(p);
        int ca = get<1>(p);
        int mw = get<2>(p);
        ExpansionHeuristic* h = new ExpansionHeuristic(mw);
        random_exp_heuristics.push_back(h);

        char nm[128];
        snprintf(nm, sizeof(nm), "Expansion_Cl%d_Ca%d_M%d", cl, ca, mw);
        stage24.push_back({string(nm), SOLVER_ID, h, 10000, 0, id_time_ms, cl, ca});
    }

    stage24.push_back({"Influence+Expansion_Cl60_Ca30_M60", SOLVER_ID, &h_infl_exp_m60, 10000, 0, id_time_ms, 60, 30});
    stage24.push_back({"Influence+Expanison_Cl300_Ca30_M60", SOLVER_ID, &h_infl_exp_m60_b, 10000, 0, id_time_ms, 300, 30});

    string csv24 = string("out/stage24_random_search_") + run_tag + ".csv";
    TournamentResult r24 = run_tournament(stage24, trials_main, csv24, report_fp, "Stage 24 - Random Search + Strong Baselines");

    vector<int> idx24(stage24.size());
    for (int i = 0; i < (int)stage24.size(); i++) idx24[i] = i;
    sort(idx24.begin(), idx24.end(), [&](int a, int b) {
        if (r24.wins[a] != r24.wins[b]) return r24.wins[a] > r24.wins[b];
        if (r24.timeout_wins[a] != r24.timeout_wins[b]) return r24.timeout_wins[a] < r24.timeout_wins[b];
        return r24.avg_move_time[a] < r24.avg_move_time[b];
    });

    fprintf(report_fp, "## Selection Summary\n\n");
    if (!idx24.empty()) {
        int best = idx24[0];
        fprintf(report_fp, "Stage 24 best: **%s** (wins=%d, timeout wins=%d, avg move time=%.2f ms).\n\n",
                stage24[best].name.c_str(), r24.wins[best], r24.timeout_wins[best], r24.avg_move_time[best]);
    }
    fprintf(report_fp, "Search space (random on Expansion): clone base in [40,320], capture weight in [10,50], material weight in [20,100].\n");
    fprintf(report_fp, "Fixed setting: jump base = 100.\n");
    fprintf(report_fp, "Comparison includes known strong models: Expansion_Cl60_Ca30_M70, Influence+Expansion_Cl60_Ca30_M60, Influence+Expanison_Cl300_Ca30_M60.\n");
    fprintf(report_fp, "Config: ID only, time limit 1000ms, trials=6 per pair.\n\n");

    fclose(report_fp);

    printf("Stage 24 run completed.\n");
    printf("Report: %s\n", report_path.c_str());
    printf("CSV 24: %s\n", csv24.c_str());

    return 0;
}

#endif
