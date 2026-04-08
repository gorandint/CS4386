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
#include <vector>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

using namespace std;

#ifndef MAX_M
#define MAX_M 7
#define MAX_N 7
#define MAX_PIECES 49
#endif

static_assert(MAX_M == 7 && MAX_N == 7, "This implementation targets a 7x7 Ataxx variant.");

typedef unsigned long long U64;
typedef signed char int8;

const int WIN_SCORE = 100000000;
const int INF_SCORE = 200000000;
const int MAX_GAME_PLY = 200;
const int MAX_DEPTH = 64;

enum TTFlag { TT_EXACT = 0, TT_LOWER = 1, TT_UPPER = 2 };

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

struct State {
    U64 p1;
    U64 p2;
    int16_t ply;
    int8 pass_count;

    State() : p1(0ULL), p2(0ULL), ply(0), pass_count(0) {}
};

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
static TTEntry g_tt[TT_SIZE];
static Move g_killer[MAX_DEPTH][2];

static U64 g_valid_mask = 0ULL;
static array<U64, 64> g_adj_mask;
static array<U64, 64> g_clone_dst_mask;
static array<U64, 64> g_jump_dst_mask;

static U64 g_zob[64][3];
static U64 g_zob_side;
static U64 g_zob_ply[MAX_GAME_PLY + 1];
static U64 g_zob_pass[3];
static bool g_zob_ready = false;

static bool g_time_out = false;
static chrono::steady_clock::time_point g_start_time;
static double g_time_limit_ms = 0.0;

static mt19937_64 g_rng(
    (uint64_t)chrono::steady_clock::now().time_since_epoch().count() ^
    0x9e3779b97f4a7c15ULL
);

inline int lsb_index(U64 x) {
    return __builtin_ctzll(x);
}

inline int popcnt(U64 x) {
    return __builtin_popcountll(x);
}

inline U64 bit_at(int r, int c) {
    return 1ULL << (r * 8 + c);
}

inline int8 piece_at(const State& s, int r, int c) {
    U64 b = bit_at(r, c);
    if (s.p1 & b) return 1;
    if (s.p2 & b) return 2;
    return 0;
}

inline int center_dist2(int r, int c) {
    int dr = r - 3;
    int dc = c - 3;
    return dr * dr + dc * dc;
}

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
    for (int i = 0; i <= MAX_GAME_PLY; i++) g_zob_ply[i] = g_rng();
    for (int i = 0; i < 3; i++) g_zob_pass[i] = g_rng();
    g_zob_ready = true;
}

inline U64 state_hash(const State& s, int8 player) {
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
    if (s.ply >= 0 && s.ply <= MAX_GAME_PLY) h ^= g_zob_ply[s.ply];
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

inline int evaluate_terminal(const State& s, int8 player) {
    int p1 = popcnt(s.p1);
    int p2 = popcnt(s.p2);
    if (p1 == 0) return (player == 2 ? WIN_SCORE : -WIN_SCORE);
    if (p2 == 0) return (player == 1 ? WIN_SCORE : -WIN_SCORE);

    U64 occ = s.p1 | s.p2;
    if (occ == g_valid_mask || s.ply >= MAX_GAME_PLY || s.pass_count >= 2) {
        if (p1 > p2) return (player == 1 ? WIN_SCORE : -WIN_SCORE);
        if (p2 > p1) return (player == 2 ? WIN_SCORE : -WIN_SCORE);
        return 0;
    }
    return INF_SCORE;
}

void generate_moves(const State& s, int8 player, vector<Move>& moves, bool any_one = false) {
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
            Move mv((int8)sr, (int8)sc, (int8)dr, (int8)dc, 1);

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
            Move mv((int8)sr, (int8)sc, (int8)dr, (int8)dc, 0);

            U64 cap_mask = g_adj_mask[didx] & (player == 1 ? s.p2 : s.p1);
            int cap = popcnt(cap_mask);
            mv.pri = 100 + cap * 30 - center_dist2(dr, dc);
            moves.push_back(mv);
            if (any_one) return;
        }
    }
}

State apply_move(const State& s, int8 player, const Move& mv) {
    State ns = s;
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

State apply_pass(const State& s) {
    State ns = s;
    ns.ply++;
    ns.pass_count = (int8)min(2, (int)s.pass_count + 1);
    return ns;
}

class Heuristic {
public:
    virtual ~Heuristic() {}
    virtual const char* name() const = 0;
    virtual int estimate(const State& s) const = 0; // positive means P1 better

    int evaluate(const State& s, int8 player) const {
        int terminal = evaluate_terminal(s, player);
        if (terminal != INF_SCORE) return terminal;
        int val = estimate(s);
        return (player == 1 ? val : -val);
    }
};

class MaterialHeuristic : public Heuristic {
public:
    const char* name() const { return "Material"; }
    int estimate(const State& s) const {
        return (popcnt(s.p1) - popcnt(s.p2)) * 100;
    }
};

class MobilityHeuristic : public Heuristic {
public:
    const char* name() const { return "Mobility"; }
    int estimate(const State& s) const {
        vector<Move> m1, m2;
        generate_moves(s, 1, m1);
        generate_moves(s, 2, m2);
        return (popcnt(s.p1) - popcnt(s.p2)) * 80 + ((int)m1.size() - (int)m2.size()) * 6;
    }
};

class CenterControlHeuristic : public Heuristic {
public:
    const char* name() const { return "CenterControl"; }
    int estimate(const State& s) const {
        int score = 0;
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
    int estimate(const State& s) const {
        int score = (popcnt(s.p1) - popcnt(s.p2)) * 70;

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
    int estimate(const State& s) const {
        int score = (popcnt(s.p1) - popcnt(s.p2)) * 60;
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
    int estimate(const State& s) const {
        int score = (popcnt(s.p1) - popcnt(s.p2)) * 80;
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
    int estimate(const State& s) const {
        int score = (popcnt(s.p1) - popcnt(s.p2)) * 70;
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
    int estimate(const State& s) const {
        int p1n = popcnt(s.p1), p2n = popcnt(s.p2);
        int empties = 49 - p1n - p2n;
        int score = (p1n - p2n) * 90;

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
    int estimate(const State& s) const {
        int score = (popcnt(s.p1) - popcnt(s.p2)) * 100;
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

class MaterialBoostHeuristic : public Heuristic {
private:
    const Heuristic* base_;
    int material_w_;
    string name_;

public:
    MaterialBoostHeuristic(const Heuristic* base, int material_w)
        : base_(base), material_w_(material_w) {
        name_ = string(base_->name()) + "+Mat" + to_string(material_w_);
    }

    const char* name() const { return name_.c_str(); }

    int estimate(const State& s) const {
        int mat = popcnt(s.p1) - popcnt(s.p2);
        return base_->estimate(s) + material_w_ * mat;
    }
};

class AdditiveBlendHeuristic : public Heuristic {
private:
    const Heuristic* a_;
    const Heuristic* b_;
    int w_b_;
    int scale_;
    string name_;

public:
    AdditiveBlendHeuristic(const Heuristic* a, const Heuristic* b, int w_b, int scale = 100)
        : a_(a), b_(b), w_b_(w_b), scale_(scale) {
        name_ = string(a_->name()) + "+" + b_->name() + "x" + to_string(w_b_);
    }

    const char* name() const { return name_.c_str(); }

    int estimate(const State& s) const {
        return a_->estimate(s) + (b_->estimate(s) * w_b_) / scale_;
    }
};

void order_moves(vector<Move>& moves, int depth, const Move* tt_best) {
    for (Move& mv : moves) {
        if (tt_best && mv == *tt_best) mv.pri += 2000000;
        if (depth < MAX_DEPTH && mv == g_killer[depth][0]) mv.pri += 500000;
        else if (depth < MAX_DEPTH && mv == g_killer[depth][1]) mv.pri += 300000;
    }
    sort(moves.begin(), moves.end());
}

int ab_negamax(const State& s, U64 hash, int depth, int max_depth, int8 player,
               const Heuristic* h, int alpha, int beta) {
    static int node_count = 0;
    if (g_time_limit_ms > 0.0 && ((++node_count & 8191) == 0)) {
        double elapsed = chrono::duration<double, milli>(
            chrono::steady_clock::now() - g_start_time
        ).count();
        if (elapsed > g_time_limit_ms) g_time_out = true;
    }
    if (g_time_out) return 0;

    int term = evaluate_terminal(s, player);
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
        State ns = apply_pass(s);
        U64 nh = state_hash(ns, (int8)(3 - player));
        return -ab_negamax(ns, nh, depth + 1, max_depth, (int8)(3 - player), h, -beta, -alpha);
    }

    order_moves(moves, depth, has_tt_best ? &tt_best : nullptr);

    int best_score = -INF_SCORE;
    Move best_move = moves[0];
    int alpha0 = alpha;
    int flag = TT_UPPER;

    for (const Move& mv : moves) {
        State ns = apply_move(s, player, mv);
        U64 nh = state_hash(ns, (int8)(3 - player));
        int score = -ab_negamax(ns, nh, depth + 1, max_depth, (int8)(3 - player), h, -beta, -alpha);
        if (g_time_out) return 0;

        if (score > best_score) {
            best_score = score;
            best_move = mv;
        }
        if (score > alpha) {
            alpha = score;
            flag = TT_EXACT;
        }
        if (alpha >= beta) {
            flag = TT_LOWER;
            killer_update(depth, mv);
            break;
        }
    }

    if (best_score <= alpha0) flag = TT_UPPER;
    tt_store(hash, (int8)remain, (int8)flag, best_score, best_move);
    return best_score;
}

Move ab_solver(const State& s, int8 player, const Heuristic* h, int max_depth) {
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
        State ns = apply_move(s, player, mv);
        U64 nh = state_hash(ns, (int8)(3 - player));
        int score = -ab_negamax(ns, nh, 1, max_depth, (int8)(3 - player), h, -beta, -alpha);
        if (score > alpha) {
            alpha = score;
            best = mv;
        }
    }
    (void)hash;
    return best;
}

Move iterative_deepening_solver(const State& s, int8 player, const Heuristic* h,
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

    for (int depth = 1; depth <= max_depth; depth++) {
        if (g_time_out) break;

        for (int i = 0; i < (int)root_moves.size(); i++) {
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
            State ns = apply_move(s, player, mv);
            U64 nh = state_hash(ns, (int8)(3 - player));
            int score = -ab_negamax(
                ns, nh, 1, depth, (int8)(3 - player), h,
                -beta, -max(alpha, best_score_depth)
            );

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

    (void)root_hash;
    return best;
}

struct MCTSNode {
    State s;
    int8 player;
    Move from_parent;
    MCTSNode* parent;
    vector<MCTSNode*> children;
    vector<Move> untried;
    int visits;
    double win_sum;

    MCTSNode(const State& _s, int8 _player, const Move& mv, MCTSNode* p)
        : s(_s), player(_player), from_parent(mv), parent(p), visits(0), win_sum(0.0) {
        generate_moves(s, player, untried);
    }

    ~MCTSNode() {
        for (MCTSNode* c : children) delete c;
    }

    bool terminal() const {
        return evaluate_terminal(s, player) != INF_SCORE;
    }
};

int winner_from_state(const State& s) {
    int p1 = popcnt(s.p1);
    int p2 = popcnt(s.p2);
    if (p1 == 0) return 2;
    if (p2 == 0) return 1;
    U64 occ = s.p1 | s.p2;
    if (occ == g_valid_mask || s.ply >= MAX_GAME_PLY || s.pass_count >= 2) {
        if (p1 > p2) return 1;
        if (p2 > p1) return 2;
        return 0;
    }
    return -1;
}

Move choose_playout_move(const State& s, int8 player, const Heuristic* h, double eps) {
    vector<Move> moves;
    generate_moves(s, player, moves);
    if (moves.empty()) return Move();

    double r = (double)(g_rng() % 1000000) / 1000000.0;
    if (h == nullptr || r < eps) {
        return moves[(size_t)(g_rng() % moves.size())];
    }

    int best = -INF_SCORE;
    Move best_mv = moves[0];
    for (const Move& mv : moves) {
        State ns = apply_move(s, player, mv);
        int v = h->evaluate(ns, player);
        if (v > best) {
            best = v;
            best_mv = mv;
        }
    }
    return best_mv;
}

int mcts_playout(State s, int8 player, const Heuristic* h, int root_player, double eps = 0.65) {
    for (int t = 0; t < 220; t++) {
        int w = winner_from_state(s);
        if (w != -1) return w;

        vector<Move> moves;
        generate_moves(s, player, moves);
        if (moves.empty()) {
            s = apply_pass(s);
            player = (int8)(3 - player);
            continue;
        }

        Move mv = choose_playout_move(s, player, h, eps);
        s = apply_move(s, player, mv);
        player = (int8)(3 - player);
    }

    int p1 = popcnt(s.p1), p2 = popcnt(s.p2);
    if (p1 > p2) return 1;
    if (p2 > p1) return 2;
    return root_player;
}

Move mcts_solver(const State& root_state, int8 root_player, const Heuristic* h,
                 int max_iters, double time_limit_ms) {
    MCTSNode* root = new MCTSNode(root_state, root_player, Move(), nullptr);
    if (root->untried.empty()) {
        delete root;
        return Move();
    }

    auto t0 = chrono::steady_clock::now();
    const double C = 1.41421356237;

    for (int iter = 0; iter < max_iters; iter++) {
        if ((iter & 127) == 0 && time_limit_ms > 0.0) {
            double elapsed = chrono::duration<double, milli>(chrono::steady_clock::now() - t0).count();
            if (elapsed > time_limit_ms) break;
        }

        MCTSNode* node = root;

        while (node->untried.empty() && !node->children.empty() && !node->terminal()) {
            MCTSNode* best = nullptr;
            double best_uct = -1e100;
            double ln_vis = log((double)max(1, node->visits));
            for (MCTSNode* ch : node->children) {
                double exploit = (ch->visits > 0 ? ch->win_sum / ch->visits : 0.0);
                double explore = C * sqrt(ln_vis / (ch->visits + 1e-9));
                double uct = exploit + explore;
                if (uct > best_uct) {
                    best_uct = uct;
                    best = ch;
                }
            }
            node = best;
        }

        if (!node->terminal() && !node->untried.empty()) {
            int pick = (int)(g_rng() % node->untried.size());
            Move mv = node->untried[pick];
            node->untried[pick] = node->untried.back();
            node->untried.pop_back();

            State ns = apply_move(node->s, node->player, mv);
            MCTSNode* child = new MCTSNode(ns, (int8)(3 - node->player), mv, node);
            node->children.push_back(child);
            node = child;
        }

        int winner = mcts_playout(node->s, node->player, h, root_player);
        double result = 0.5;
        if (winner == root_player) result = 1.0;
        else if (winner == 3 - root_player) result = 0.0;

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
    delete root;
    return ans;
}

State board_to_state(const int board[MAX_M][MAX_N], int step) {
    State s;
    for (int r = 0; r < 7; r++) {
        for (int c = 0; c < 7; c++) {
            U64 b = bit_at(r, c);
            if (board[r][c] == 1) s.p1 |= b;
            else if (board[r][c] == 2) s.p2 |= b;
        }
    }
    s.ply = (int16_t)(step - 1);
    s.pass_count = 0;
    return s;
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
    State s = board_to_state(board, step);

    static ExpansionHeuristic h_exp;
    static MaterialBoostHeuristic h_best(&h_exp, 130);
    Move best = iterative_deepening_solver(s, player, &h_best, 10, 1850.0);

    vector<Move> sanity;
    generate_moves(s, player, sanity, true);
    if (sanity.empty()) {
        save_decision(0, 0, 0, 0);
        return;
    }
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

    Move get_move(const State& s, int8 player) const {
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
    vector<double> avg_move_time;
};

static string make_unique_tag() {
    long long ms = chrono::duration_cast<chrono::milliseconds>(
        chrono::system_clock::now().time_since_epoch()
    ).count();
    return to_string(ms);
}

static void ensure_out_dir() {
#ifdef _WIN32
    _mkdir("out");
#else
    mkdir("out", 0755);
#endif
}

State initial_state() {
    State s;
    s.p1 = bit_at(0, 0) | bit_at(6, 6);
    s.p2 = bit_at(0, 6) | bit_at(6, 0);
    s.ply = 0;
    s.pass_count = 0;
    return s;
}

MatchResult play_match(const SolverProfile& p1, const SolverProfile& p2, int max_ply = MAX_GAME_PLY) {
    State s = initial_state();
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
    int n = (int)solvers.size();
    vector<int> wins(n, 0);
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
        return avg_time[a] < avg_time[b];
    });

    fprintf(report_fp, "## %s\n\n", stage_name);
    fprintf(report_fp, "- CSV: %s\n", csv_path.c_str());
    fprintf(report_fp, "- Format: iterative deepening only, time limit 1000 ms, 10 trials per pair (color swapped by trial parity).\n\n");
    fprintf(report_fp, "| Rank | Heuristic | Wins | Avg Move Time (ms) |\n");
    fprintf(report_fp, "|---:|---|---:|---:|\n");
    for (int k = 0; k < n; k++) {
        int i = rank[k];
        fprintf(report_fp, "| %d | %s | %d | %.2f |\n", k + 1, solvers[i].name, wins[i], avg_time[i]);
    }
    fprintf(report_fp, "\n");

    return {wins, avg_time};
}

int main() {
    init_masks();
    init_zobrist();

    ensure_out_dir();

    string run_tag = make_unique_tag();
    string report_path = string("out/heuristic_iteration_report_") + run_tag + ".md";
    FILE* report_fp = fopen(report_path.c_str(), "w");

    fprintf(report_fp, "# Ataxx Heuristic Iteration Report\n\n");
    fprintf(report_fp, "Run tag: %s\n\n", run_tag.c_str());
    fprintf(report_fp, "## Heuristic Ideas\n\n");
    fprintf(report_fp, "- Material: maximize piece count gap.\n");
    fprintf(report_fp, "- Mobility: favor positions with more legal actions.\n");
    fprintf(report_fp, "- CenterControl: keep pieces near center for flexible expansion.\n");
    fprintf(report_fp, "- InfectionPressure: reward adjacency to enemy stones for conversion potential.\n");
    fprintf(report_fp, "- Expansion: maximize reachable empty cells by clone/jump.\n");
    fprintf(report_fp, "- Safety: reward local friendly support and penalize local enemy pressure.\n");
    fprintf(report_fp, "- Influence: value empty cells by nearby friendly minus enemy neighbors.\n");
    fprintf(report_fp, "- Frontier: dynamically value frontier size by game phase.\n");
    fprintf(report_fp, "- Hybrid: combine material + center + pressure + expansion.\n\n");

    MaterialHeuristic h_mat;
    MobilityHeuristic h_mob;
    CenterControlHeuristic h_ctr;
    InfectionPressureHeuristic h_inf;
    ExpansionHeuristic h_exp;
    SafetyHeuristic h_safe;
    InfluenceHeuristic h_infl;
    FrontierHeuristic h_front;
    HybridHeuristic h_hyb;

    const int trials = 10;
    const int id_depth = 6;
    const double id_time_ms = 1000.0;

    vector<const Heuristic*> base_h = {
        &h_mat, &h_mob, &h_ctr, &h_inf, &h_exp, &h_hyb
    };

    vector<SolverProfile> stage1;
    for (const Heuristic* h : base_h) {
        stage1.push_back({h->name(), SOLVER_ID, h, id_depth, 0, id_time_ms});
    }

    string csv1 = string("out/stage1_base_") + run_tag + ".csv";
    TournamentResult r1 = run_tournament(stage1, trials, csv1, report_fp, "Stage 1 - Base Heuristics");

    vector<int> idx1(stage1.size());
    for (int i = 0; i < (int)stage1.size(); i++) idx1[i] = i;
    sort(idx1.begin(), idx1.end(), [&](int a, int b) {
        if (r1.wins[a] != r1.wins[b]) return r1.wins[a] > r1.wins[b];
        return r1.avg_move_time[a] < r1.avg_move_time[b];
    });

    vector<const Heuristic*> top_stage1;
    for (int k = 0; k < 3 && k < (int)idx1.size(); k++) top_stage1.push_back(stage1[idx1[k]].h);

    vector<MaterialBoostHeuristic> boosted;
    boosted.reserve(top_stage1.size() * 3);
    const int mat_weights[2] = {80, 120};
    for (const Heuristic* h : top_stage1) {
        for (int mw : mat_weights) boosted.emplace_back(h, mw);
    }

    vector<SolverProfile> stage2;
    for (const Heuristic* h : top_stage1) {
        stage2.push_back({h->name(), SOLVER_ID, h, id_depth, 0, id_time_ms});
    }
    for (const MaterialBoostHeuristic& h : boosted) {
        stage2.push_back({h.name(), SOLVER_ID, &h, id_depth, 0, id_time_ms});
    }

    string csv2 = string("out/stage2_material_boost_") + run_tag + ".csv";
    TournamentResult r2 = run_tournament(stage2, trials, csv2, report_fp, "Stage 2 - Material Boost Variants");

    vector<int> idx2(stage2.size());
    for (int i = 0; i < (int)stage2.size(); i++) idx2[i] = i;
    sort(idx2.begin(), idx2.end(), [&](int a, int b) {
        if (r2.wins[a] != r2.wins[b]) return r2.wins[a] > r2.wins[b];
        return r2.avg_move_time[a] < r2.avg_move_time[b];
    });

    vector<const Heuristic*> finalists;
    for (int k = 0; k < 4 && k < (int)idx2.size(); k++) finalists.push_back(stage2[idx2[k]].h);

    vector<MaterialBoostHeuristic> final_boost;
    final_boost.reserve(finalists.size());
    for (const Heuristic* h : finalists) final_boost.emplace_back(h, 100);

    vector<SolverProfile> stage3;
    for (const Heuristic* h : finalists) {
        stage3.push_back({h->name(), SOLVER_ID, h, id_depth, 0, id_time_ms});
    }
    for (const MaterialBoostHeuristic& h : final_boost) {
        stage3.push_back({h.name(), SOLVER_ID, &h, id_depth, 0, id_time_ms});
    }

    string csv3 = string("out/stage3_finalists_") + run_tag + ".csv";
    TournamentResult r3 = run_tournament(stage3, trials, csv3, report_fp, "Stage 3 - Finalists Re-Iteration");

    vector<int> idx3(stage3.size());
    for (int i = 0; i < (int)stage3.size(); i++) idx3[i] = i;
    sort(idx3.begin(), idx3.end(), [&](int a, int b) {
        if (r3.wins[a] != r3.wins[b]) return r3.wins[a] > r3.wins[b];
        return r3.avg_move_time[a] < r3.avg_move_time[b];
    });

    fprintf(report_fp, "## Final Selection\n\n");
    if (!idx3.empty()) {
        int best = idx3[0];
        fprintf(report_fp, "Best heuristic in this run: **%s** (wins=%d, avg move time=%.2f ms).\n\n",
                stage3[best].name, r3.wins[best], r3.avg_move_time[best]);
    }
    fprintf(report_fp, "Observations:\n\n");
    fprintf(report_fp, "- Material remains a strong baseline in this variant due to conversion snowballing.\n");
    fprintf(report_fp, "- Mobility and infection-aware terms usually help in midgame tactical fights.\n");
    fprintf(report_fp, "- Material boosting (+Mat100 around top heuristics) is often a robust upgrade.\n");
    fprintf(report_fp, "- Frontier terms are phase-sensitive and can underperform if over-weighted.\n");
    fprintf(report_fp, "\n");

    vector<MaterialBoostHeuristic> exp_weight_scan;
    const int tuned_mat_w[4] = {90, 110, 130, 150};
    exp_weight_scan.reserve(4);
    for (int w : tuned_mat_w) exp_weight_scan.emplace_back(&h_exp, w);

    vector<AdditiveBlendHeuristic> exp_inf_blend;
    exp_inf_blend.reserve(2);
    exp_inf_blend.emplace_back(&h_exp, &h_inf, 20);
    exp_inf_blend.emplace_back(&h_exp, &h_inf, 35);

    vector<MaterialBoostHeuristic> exp_inf_blend_boost;
    exp_inf_blend_boost.reserve(exp_inf_blend.size());
    for (const AdditiveBlendHeuristic& h : exp_inf_blend) {
        exp_inf_blend_boost.emplace_back(&h, 100);
    }

    vector<SolverProfile> stage4;
    stage4.push_back({"Expansion", SOLVER_ID, &h_exp, id_depth, 0, id_time_ms});
    stage4.push_back({"Hybrid", SOLVER_ID, &h_hyb, id_depth, 0, id_time_ms});
    stage4.push_back({"Material", SOLVER_ID, &h_mat, id_depth, 0, id_time_ms});
    for (const MaterialBoostHeuristic& h : exp_weight_scan) {
        stage4.push_back({h.name(), SOLVER_ID, &h, id_depth, 0, id_time_ms});
    }
    for (const AdditiveBlendHeuristic& h : exp_inf_blend) {
        stage4.push_back({h.name(), SOLVER_ID, &h, id_depth, 0, id_time_ms});
    }
    for (const MaterialBoostHeuristic& h : exp_inf_blend_boost) {
        stage4.push_back({h.name(), SOLVER_ID, &h, id_depth, 0, id_time_ms});
    }

    string csv4 = string("out/stage4_targeted_") + run_tag + ".csv";
    TournamentResult r4 = run_tournament(stage4, trials, csv4, report_fp, "Stage 4 - Targeted Expansion Tuning");

    vector<int> idx4(stage4.size());
    for (int i = 0; i < (int)stage4.size(); i++) idx4[i] = i;
    sort(idx4.begin(), idx4.end(), [&](int a, int b) {
        if (r4.wins[a] != r4.wins[b]) return r4.wins[a] > r4.wins[b];
        return r4.avg_move_time[a] < r4.avg_move_time[b];
    });

    fprintf(report_fp, "## Targeted Tuning Conclusion\n\n");
    if (!idx4.empty()) {
        int best4 = idx4[0];
        fprintf(report_fp, "Top heuristic in targeted stage: **%s** (wins=%d, avg move time=%.2f ms).\n\n",
                stage4[best4].name, r4.wins[best4], r4.avg_move_time[best4]);
    }

    fclose(report_fp);

    printf("Autopilot run completed.\n");
    printf("Report: %s\n", report_path.c_str());
    printf("CSV 1: %s\n", csv1.c_str());
    printf("CSV 2: %s\n", csv2.c_str());
    printf("CSV 3: %s\n", csv3.c_str());
    printf("CSV 4: %s\n", csv4.c_str());

    return 0;
}

#endif
