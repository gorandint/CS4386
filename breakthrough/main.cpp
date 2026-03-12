#ifndef LOCAL
#include "battle_base.h"
#endif

#include <vector>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <algorithm>
#include <vector>
#include <cstdio>
#include <chrono>
#include <random>

using namespace std;

#ifndef MAX_M
#define MAX_M 8
#define MAX_N 8
#define MAX_PIECES 16
#endif

const int MY_NEG_INF = -100000000;
const int MY_POS_INF =  100000000;
const int MY_NEAR_INF = 90000000;

typedef char int8;
typedef unsigned long long U64;

#define lowbit_id __builtin_ctzll // get lowest 1 bit index
#define popcount __builtin_popcountll // count 1 bits
#define lowbit_pop(x) (x = (x & (x - 1))) // remove lowest 1 bit

bool g_time_out = false;
chrono::steady_clock::time_point g_start_time;
double g_time_limit_ms = 0;

// bit representation

struct BitBoard {
    U64 z[2]; // [0=p1, 1=p2]

    BitBoard() {
        z[0] = z[1] = 0ULL;
    }

    BitBoard(U64 p1, U64 p2) : z{p1, p2} {}

    BitBoard(int grid[MAX_M][MAX_N]) {
        z[0] = z[1] = 0ULL;
        for (int8 r = 0; r < MAX_M; r++)
            for (int8 c = 0; c < MAX_N; c++) {
                int8 idx = r * MAX_N + c;
                if (grid[r][c] == 1) z[0] |= (1ULL << idx);
                else if (grid[r][c] == 2) z[1] |= (1ULL << idx);
            }
    }

    // error: passing 'const BitBoard' as 'this' argument discards qualifiers [-fpermissive]
    // had to add const to read-only get() and set()
    inline int get(int8 r, int8 c) const {
        U64 mask = 1ULL << (r * MAX_N + c);
        if (z[0] & mask) return 1;
        if (z[1] & mask) return 2;
        return 0;
    }

    inline void set(int8 r, int8 c, int8 val) {
        U64 mask = 1ULL << (r * MAX_N + c);
        if (val == 1) {
            z[0] |= mask;
            z[1] &= ~mask;
        } else if (val == 2) {
            z[1] |= mask;
            z[0] &= ~mask;
        } else {
            z[0] &= ~mask;
            z[1] &= ~mask;
        }
    }

    inline void clear(int8 r, int8 c) {
        U64 mask = 1ULL << (r * MAX_N + c);
        z[0] &= ~mask;
        z[1] &= ~mask;
    }

    inline U64 get_p(int8 player) const {
        return z[player - 1];
    }

    void to_array(int grid[MAX_M][MAX_N]) const {
        for (int8 r = 0; r < MAX_M; r++) {
            for (int8 c = 0; c < MAX_N; c++) {
                int8 idx = r * MAX_N + c;
                if (z[0] & (1ULL << idx)) grid[r][c] = 1;
                else if (z[1] & (1ULL << idx)) grid[r][c] = 2;
                else grid[r][c] = 0;
            }
        }
    }
};

const U64 ROW_MASKS[MAX_M] = {
    0xFFULL << (0 * MAX_N), // 0000 0000 0000 00FF
    0xFFULL << (1 * MAX_N),
    0xFFULL << (2 * MAX_N),
    0xFFULL << (3 * MAX_N),
    0xFFULL << (4 * MAX_N),
    0xFFULL << (5 * MAX_N),
    0xFFULL << (6 * MAX_N),
    0xFFULL << (7 * MAX_N), // FF00 0000 0000 0000
};

const U64 DL_MASK = ~0x0101010101010101ULL; // col 0 pawns can't move diagonally left
const U64 DR_MASK = ~0x8080808080808080ULL; // col 7

struct Move {
    int8 src_r, src_c;
    int8 dst_r, dst_c;
    int pri;
    Move() : src_r(0), src_c(0), dst_r(0), dst_c(0), pri(0) {}
    Move(int8 sr, int8 sc, int8 dr, int8 dc)
        : src_r(sr), src_c(sc), dst_r(dr), dst_c(dc), pri(0) {}
};

bool operator==(const Move& a, const Move& b) {
    return a.src_r == b.src_r && a.src_c == b.src_c && a.dst_r == b.dst_r && a.dst_c == b.dst_c;
}

bool operator <(const Move& a, const Move& b) {
    return a.pri > b.pri; // higher priority first
}

inline int8 get_row(int8 idx) {
    return idx / MAX_N;
}

inline int8 get_col(int8 idx) {
    return idx % MAX_N;
}

inline U64 get_bit(int8 r, int8 c) {
    return 1ULL << (r * MAX_N + c);
}

inline U64 col_mask(int8 c) {
    U64 m = 0;
    for (int8 r = 0; r < MAX_M; r++) m |= get_bit(r, c);
    return m;
}

inline U64 lane_mask(int8 c) { // left + center + right columns
    U64 m = col_mask(c);
    if (c > 0) m |= col_mask(c - 1);
    if (c < MAX_N - 1) m |= col_mask(c + 1);
    return m;
}

inline U64 front_mask_p1(int8 r) { // rows from r+1 to end (7) for P1
    U64 m = 0;
    for (int8 rr = r + 1; rr < MAX_M; rr++) m |= ROW_MASKS[rr];
    return m;
}

inline U64 front_mask_p2(int8 r) {
    U64 m = 0;
    for (int8 rr = 0; rr < r; rr++) m |= ROW_MASKS[rr];
    return m;
}

// Zobrist hashing

U64 zobrist_table[MAX_M][MAX_N][3]; // [r][c][0=empty,1=p1,2=p2]
// [0] is not used, just no need to handle index [player-1]
U64 zobrist_flip; // flip side after each move
bool zobrist_init_done = false;
random_device rd;
mt19937_64 global_rng(rd());

void init_zobrist() {
    if (zobrist_init_done) return;
    memset(zobrist_table, 0, sizeof(zobrist_table));
    for (int8 r = 0; r < MAX_M; r++)
        for (int8 c = 0; c < MAX_N; c++)
            for (int8 p = 1; p < 3; p++)
                zobrist_table[r][c][p] = global_rng();
    zobrist_flip = global_rng();
    zobrist_init_done = true;
}

U64 get_hash(const BitBoard& b, int8 player) {
    U64 hash = 0;
    for (int8 r = 0; r < MAX_M; r++)
        for (int8 c = 0; c < MAX_N; c++) {
            hash ^= zobrist_table[r][c][b.get(r, c)];
        }
    if (player == 2) hash ^= zobrist_flip;
    return hash;
}

U64 incremental_hash(U64 hash, int8 sr, int8 sc, int8 sp, int8 dr, int8 dc, int8 dp) {
    hash ^= zobrist_table[sr][sc][sp]; // empty source
    hash ^= zobrist_table[dr][dc][dp]; // empty destination
    hash ^= zobrist_table[dr][dc][sp]; // new piece at destination
    hash ^= zobrist_flip; // flip side
    return hash;
}

// Transposition table

enum TTFlag { EXACT, LOWERBOUND, UPPERBOUND };

struct TTEntry {
    U64 hash;
    int8 depth; // remaining depth when stored, so prefer deeper entries
    TTFlag flag;
    int score;
    Move best_move;
    int8 valid;

    TTEntry() : hash(0), depth(0), flag(EXACT), score(0), valid(0) {}
};

const int TT_SIZE = 1 << 18;
const int TT_MASK = TT_SIZE - 1;
TTEntry tt[TT_SIZE];
// size: (4+1+1+2+5+1) = 14 bytes per entry, * 262144 entries = ~3.6MB

void tt_clear() {
    memset(tt, 0, sizeof(tt));
}

TTEntry* tt_get(U64 hash) {
    TTEntry* e = &tt[hash & TT_MASK];
    if (e->valid && e->hash == hash) return e;
    return nullptr;
}

void tt_update(U64 h, int8 depth, TTFlag flag, int score, Move best_move) {
    TTEntry& e = tt[h & TT_MASK];
    // replace if (1) collision (2) current search is deeper
    if (!e.valid || e.depth <= depth || e.hash == h) {
        e.hash = h;
        e.depth = depth;
        e.flag = flag;
        e.score = score;
        e.best_move = best_move;
        e.valid = 1;
    }
}

// Killer move

const int MAX_DEPTH = 64;
Move killer_moves[MAX_DEPTH][2];

void killer_clear() {
    memset(killer_moves, 0, sizeof(killer_moves));
}

void killer_update(int8 depth, Move mv) {
    if (depth >= MAX_DEPTH) return;
    if (killer_moves[depth][0] == mv) return;
    killer_moves[depth][1] = killer_moves[depth][0];
    killer_moves[depth][0] = mv;
}

// General

// return winner (1 or 2), or 0 if no winner
int8 check_win(const BitBoard& b) {
    // any piece reach row M-1 for p1 or row 0 for p2
    if (b.get_p(1) & ROW_MASKS[MAX_M - 1]) return 1;
    if (b.get_p(2) & ROW_MASKS[0]) return 2;
    // or no pieces left for opponent
    if (!b.get_p(1)) return 2;
    if (!b.get_p(2)) return 1;
    return 0;
}

BitBoard apply_move(BitBoard b, const Move& mv) {
    int8 piece = b.get(mv.src_r, mv.src_c);
    b.clear(mv.src_r, mv.src_c);
    b.set(mv.dst_r, mv.dst_c, piece);
    return b;
}

// find all legal moves for player in this priority: winning move > capture > normal
vector<Move> get_all_moves(const BitBoard& b, int8 player, bool get_any = false, int8 depth = -1, Move* tt_best = nullptr) {
    vector<Move> moves;

    int8 direction = (player == 1) ? 1 : -1;
    int8 opponent = 3 - player;
    int8 goal_row = (player == 1) ? (MAX_M - 1) : 0;

    for (int8 i = 0; i < MAX_M; i++) {
        // starting from pieces closer to opponent side
        int sr = (player == 1) ? (MAX_M - 1 - i) : i;
        int dr = sr + direction;
        if (dr < 0 || dr >= MAX_M) continue;

        U64 row_bits = b.get_p(player) & ROW_MASKS[sr];

        // consider each piece in this row
        while (row_bits) {
            int8 idx = lowbit_id(row_bits);
            lowbit_pop(row_bits);
            int8 sc = idx % MAX_N;

            for (int delta_c = -1; delta_c < 2; delta_c++) {
                int8 dc = sc + delta_c;
                if (dc < 0 || dc >= MAX_N) continue;
                int8 dst_piece = b.get(dr, dc);
                if (delta_c == 0 && dst_piece != 0) continue; // forward blocked
                if (dst_piece == player) continue;

                Move mv(sr, sc, dr, dc);
                int pri = 0;
                if (dr == goal_row) {
                    pri = 500; // winning move
                } else {
                    if (dst_piece == opponent) pri = 100; // capture
                    pri += (player == 1 ? dr : (MAX_M - 1 - dr)); // advancement
                    
                    if (tt_best && mv == *tt_best) pri += 1000; // TT best move
                    if (depth >= 0) {
                        if (mv == killer_moves[depth][0]) pri += 90; // killer move
                        else if (mv == killer_moves[depth][1]) pri += 80;
                    }
                }
                mv.pri = pri;
                moves.push_back(mv);
                if (get_any) return moves;
            }
        }
    }

    sort(moves.begin(), moves.end());
    return moves;
}

// heuristics

class Heuristic {
public:
    virtual const char* name() = 0;
    virtual int estimate(const BitBoard& b) = 0;
    int evaluate(const BitBoard& b, int8 player) {
        int8 winner = check_win(b);
        if (winner == player) return MY_POS_INF;
        else if (winner != 0) return MY_NEG_INF;
        return estimate(b) * (player == 1 ? 1 : -1);
    }
};

class RandomHeuristic : public Heuristic {
public:
    const char* name() { return "Random"; }
    int estimate(const BitBoard& b) {
        return global_rng() % 201 - 100; // [-100, 100]
    }
};

class MaterialHeuristic : public Heuristic {
public:
    const char* name() { return "Material"; }
    int estimate(const BitBoard& b) {
        return popcount(b.get_p(1)) - popcount(b.get_p(2));
    }
};

class AdvancementHeuristic : public Heuristic {
public:
    const char* name() { return "Advancement"; }
    int estimate(const BitBoard& b) {
        int score = 0;
        U64 tmp = b.get_p(1);
        while (tmp) {
            int8 idx = lowbit_id(tmp);
            lowbit_pop(tmp);
            score += get_row(idx);
        }
        tmp = b.get_p(2);
        while (tmp) {
            int8 idx = lowbit_id(tmp);
            lowbit_pop(tmp);
            score -= (MAX_M - 1 - get_row(idx));
        }
        return score;
    }
};

class AttackHeuristic : public Heuristic {
public:
    const char* name() { return "Attack"; }
    int estimate(const BitBoard& b) {
        int score = 0;
        U64 p1 = b.get_p(1), p2 = b.get_p(2);
        U64 tmp = p1;
        while (tmp) {
            int8 idx = lowbit_id(tmp);
            lowbit_pop(tmp);
            int8 r = get_row(idx), c = get_col(idx);
            if (r < MAX_M - 1) {
                if (c > 0 && (p2 & get_bit(r + 1, c - 1))) score++;
                if (c < MAX_N - 1 && (p2 & get_bit(r + 1, c + 1))) score++;
            }
        }
        tmp = p2;
        while (tmp) {
            int8 idx = lowbit_id(tmp);
            lowbit_pop(tmp);
            int8 r = get_row(idx), c = get_col(idx);
            if (r - 1 >= 0) {
                if (c > 0 && (p1 & get_bit(r - 1, c - 1))) score--;
                if (c < MAX_N - 1 && (p1 & get_bit(r - 1, c + 1))) score--;
            }
        }
        return score;
    }
};

class DefenseHeuristic : public Heuristic {
public:
    const char* name() { return "Defense"; }
    int estimate(const BitBoard& b) {
        int score = 0;
        U64 p1 = b.get_p(1), p2 = b.get_p(2);
        U64 tmp = p1;
        while (tmp) {
            int8 idx = lowbit_id(tmp);
            lowbit_pop(tmp);
            int8 r = get_row(idx), c = get_col(idx);
            if (r - 1 >= 0) {
                if (c > 0 && (p1 & get_bit(r - 1, c - 1))) score++;
                if (c < MAX_N - 1 && (p1 & get_bit(r - 1, c + 1))) score++;
            }
        }
        tmp = p2;
        while (tmp) {
            int8 idx = lowbit_id(tmp);
            lowbit_pop(tmp);
            int8 r = get_row(idx), c = get_col(idx);
            if (r < MAX_M - 1) {
                if (c > 0 && (p2 & get_bit(r + 1, c - 1))) score--;
                if (c < MAX_N - 1 && (p2 & get_bit(r + 1, c + 1))) score--;
            }
        }
        return score;
    }
};

const int MY_SQ[MAX_M] = {0, 1, 4, 9, 16, 25, 36, 49};

class AggresiveMixedHeuristic : public Heuristic {
public:
    const char* name() { return "AggressiveMixed"; }
    int estimate(const BitBoard& b) {
        int score = 0;
        U64 tmp = b.get_p(1);
        while (tmp) {
            int8 idx = lowbit_id(tmp);
            lowbit_pop(tmp);
            int8 r = get_row(idx);
            score += 100 + MY_SQ[r];
            if (r >= MAX_M - 2) score += 1000;
        }
        tmp = b.get_p(2);
        while (tmp) {
            int8 idx = lowbit_id(tmp);
            lowbit_pop(tmp);
            int8 r = get_row(idx);
            score -= 100 + MY_SQ[MAX_M - 1 - r];
            if (r <= 1) score -= 1000;
        }
        return score;
    }
};

class DefensiveMixedHeuristic : public Heuristic {
public:
    const char* name() { return "DefensiveMixed"; }
    int estimate(const BitBoard& b) {
        int score = 0;
        U64 p1 = b.get_p(1), p2 = b.get_p(2);
        U64 tmp = p1;
        while (tmp) {
            int8 idx = lowbit_id(tmp);
            lowbit_pop(tmp);
            int8 r = get_row(idx), c = get_col(idx);
            score += 100 + MY_SQ[r];
            if (r - 1 >= 0) {
                if (c > 0 && (p1 & get_bit(r - 1, c - 1))) score++;
                if (c < MAX_N - 1 && (p1 & get_bit(r - 1, c + 1))) score++;
            }
        }
        tmp = p2;
        while (tmp) {
            int8 idx = lowbit_id(tmp);
            lowbit_pop(tmp);
            int8 r = get_row(idx), c = get_col(idx);
            score -= 100 + MY_SQ[MAX_M - 1 - r];
            if (r < MAX_M - 1) {
                if (c > 0 && (p2 & get_bit(r + 1, c - 1))) score--;
                if (c < MAX_N - 1 && (p2 & get_bit(r + 1, c + 1))) score--;
            }
        }
        return score;
    }
};

class MovabilityHeuristic : public Heuristic {
public:
    const char* name() { return "Movability"; }
    int estimate(const BitBoard& b) {
        int score = popcount(b.get_p(1)) - popcount(b.get_p(2));
        score *= 100;

        vector<Move> p1_moves = get_all_moves(b, 1);
        vector<Move> p2_moves = get_all_moves(b, 2);
        score += (int)p1_moves.size() * 2;
        score -= (int)p2_moves.size() * 2;
        return score;
    }
};

class MovableMaterialHeuristic : public Heuristic {
public:
    const char* name() { return "MovableMaterial"; }
    int estimate(const BitBoard& b) {
        U64 p1 = b.get_p(1), p2 = b.get_p(2);
        int score = popcount(p1) - popcount(p2);
        score *= 100;
        U64 all = p1 | p2;
        // assume all p1 pieces can move forward
        // straight << 8, diagonal left << 7, diagonal right << 9
        U64 p1_st = (p1 << MAX_N) & ~all;
        U64 p1_dl = ((p1 & DL_MASK) << (MAX_N - 1)) & ~p1;
        U64 p1_dr = ((p1 & DR_MASK) << (MAX_N + 1)) & ~p1;
        score += popcount(p1_st | p1_dl | p1_dr) * 2;

        // for p2
        // straight >> 8, diagonal left >> 9, diagonal right >> 7
        U64 p2_st = (p2 >> MAX_N) & ~all;
        U64 p2_dl = ((p2 & DL_MASK) >> (MAX_N + 1)) & ~p2;
        U64 p2_dr = ((p2 & DR_MASK) >> (MAX_N - 1)) & ~p2;
        score -= popcount(p2_st | p2_dl | p2_dr) * 2;
        return score;
    }
};

class CentralControlHeuristic : public Heuristic {
private:
    static const int8 ctr_col_weight[MAX_N];
public:
    const char* name() { return "CentralControl"; }
    int estimate(const BitBoard& b) {
        int score = 0;
        U64 tmp = b.get_p(1);
        while (tmp) {
            int8 idx = lowbit_id(tmp);
            lowbit_pop(tmp);
            int8 r = get_row(idx), c = get_col(idx);
            score += 100 + MY_SQ[r] + ctr_col_weight[c] * 10;
        }
        tmp = b.get_p(2);
        while (tmp) {
            int8 idx = lowbit_id(tmp);
            lowbit_pop(tmp);
            int8 r = get_row(idx), c = get_col(idx);
            score -= 100 + MY_SQ[MAX_M - 1 - r] + ctr_col_weight[c] * 10;
        }
        return score;
    }
};

const int8 CentralControlHeuristic::ctr_col_weight[MAX_N] = {0, 1, 2, 3, 3, 2, 1, 0};

class SafeAdvancementHeuristic : public Heuristic {
public:
    const char* name() { return "SafeAdvancement"; }
    int estimate(const BitBoard& b) {
        int score = 0;
        U64 p1 = b.get_p(1), p2 = b.get_p(2);
        U64 tmp = p1;
        while (tmp) {
            int8 idx = lowbit_id(tmp);
            lowbit_pop(tmp);
            int8 r = get_row(idx), c = get_col(idx);
            bool threat = false, protect = false;
            if (r - 1 >= 0) {
                if (c > 0 && (p1 & get_bit(r - 1, c - 1))) protect = true;
                if (c < MAX_N - 1 && (p1 & get_bit(r - 1, c + 1))) protect = true;
            }
            if (r < MAX_M - 1) {
                if (c > 0 && (p2 & get_bit(r + 1, c - 1))) threat = true;
                if (c < MAX_N - 1 && (p2 & get_bit(r + 1, c + 1))) threat = true;
            }
            score += 100 + MY_SQ[r];
            if (threat && !protect) score -= 30;
            else if (threat && protect) score -= 5;
        }
        tmp = p2;
        while (tmp) {
            int8 idx = lowbit_id(tmp);
            lowbit_pop(tmp);
            int8 r = get_row(idx), c = get_col(idx);
            bool threat = false, protect = false;
            if (r < MAX_M - 1) {
                if (c > 0 && (p2 & get_bit(r + 1, c - 1))) protect = true;
                if (c < MAX_N - 1 && (p2 & get_bit(r + 1, c + 1))) protect = true;
            }
            if (r - 1 >= 0) {
                if (c > 0 && (p1 & get_bit(r - 1, c - 1))) threat = true;
                if (c < MAX_N - 1 && (p1 & get_bit(r - 1, c + 1))) threat = true;
            }
            score -= 100 + MY_SQ[MAX_M - 1 - r];
            if (threat && !protect) score += 30;
            else if (threat && protect) score += 5;
        }
        return score;
    }
};

class PassedPawnHeuristic : public Heuristic {
private:
    static const int pass_row_weight[MAX_M];
public:
    const char* name() { return "PassedPawn"; }

    int estimate(const BitBoard& b) {
        int score = 0;
        U64 p1 = b.get_p(1), p2 = b.get_p(2);
        score += (popcount(p1) - popcount(p2)) * 100;
        U64 tmp = p1;
        while (tmp) {
            int8 idx = lowbit_id(tmp);
            lowbit_pop(tmp);
            int8 r = get_row(idx), c = get_col(idx);
            U64 lane = lane_mask(c), front = front_mask_p1(r);
            bool passed = ((p2 & lane & front) == 0);
            if (passed) score += pass_row_weight[MAX_M - 1 - r];
        }
        tmp = p2;
        while (tmp) {
            int8 idx = lowbit_id(tmp);
            lowbit_pop(tmp);
            int8 r = get_row(idx), c = get_col(idx);
            U64 lane = lane_mask(c), front = front_mask_p2(r);
            bool passed = ((p1 & lane & front) == 0);
            if (passed) score -= pass_row_weight[r];
        }
        return score;
    }
};

const int PassedPawnHeuristic::pass_row_weight[MAX_M] = {50000, 8000, 2500, 900, 300, 200, 100, 50};

class ContactExchangeHeuristic : public Heuristic {
public:
    const char* name() { return "ContactExchange"; }
    int estimate(const BitBoard& b) {
        int score = 0;
        int mat_diff = popcount(b.get_p(1)) - popcount(b.get_p(2));
        U64 p1 = b.get_p(1), p2 = b.get_p(2);
        U64 tmp = p1;
        int p1_contact = 0, p2_contact = 0;
        while (tmp) {
            int8 idx = lowbit_id(tmp);
            lowbit_pop(tmp);
            int8 r = get_row(idx), c = get_col(idx);
            if (r < MAX_M - 1) {
                if (c > 0 && (p2 & get_bit(r + 1, c - 1))) p1_contact++;
                if (c < MAX_N - 1 && (p2 & get_bit(r + 1, c + 1))) p1_contact++;
            }
        }
        tmp = p2;
        while (tmp) {
            int8 idx = lowbit_id(tmp);
            lowbit_pop(tmp);
            int8 r = get_row(idx), c = get_col(idx);
            if (r - 1 >= 0) {
                if (c > 0 && (p1 & get_bit(r - 1, c - 1))) p2_contact++;
                if (c < MAX_N - 1 && (p1 & get_bit(r - 1, c + 1))) p2_contact++;
            }
        }
        score += mat_diff * 100;
        score += (mat_diff >= 0 ? 1 : -1) * (p1_contact - p2_contact) * 25;
        return score;
    }
};

class WeightedMovabilityHeuristic : public Heuristic {
public:
    const char* name() { return "WeightedMovability"; }

    int estimate(const BitBoard& b) {
        int score = 0;
        U64 p1 = b.get_p(1), p2 = b.get_p(2);
        score += (popcount(p1) - popcount(p2)) * 100;

        vector<Move> p1_moves = get_all_moves(b, 1);
        for (const Move& mv : p1_moves) {
            if (b.get(mv.dst_r, mv.dst_c) != 0) score += 6;
            else score += 10;
        }
        vector<Move> p2_moves = get_all_moves(b, 2);
        for (const Move& mv : p2_moves) {
            if (b.get(mv.dst_r, mv.dst_c) != 0) score -= 6;
            else score -= 10;
        }
        return score;
    }
};

class CentralControlv2Heuristic : public Heuristic {
private:
    static const int8 ctr2_col_weight[MAX_N];
public:
    const char* name() { return "CentralControlv2"; }
    int estimate(const BitBoard& b) {
        int score = 0;
        U64 p1 = b.get_p(1), p2 = b.get_p(2);
        U64 tmp = p1;
        while (tmp) {
            int8 idx = lowbit_id(tmp);
            lowbit_pop(tmp);
            int8 r = get_row(idx), c = get_col(idx);
            score += 100 + r * 8 + ctr2_col_weight[c];
        }
        tmp = p2;
        while (tmp) {
            int8 idx = lowbit_id(tmp);
            lowbit_pop(tmp);
            int8 r = get_row(idx), c = get_col(idx);
            int8 d = MAX_M - 1 - r;
            score -= 100 + d * 8 + ctr2_col_weight[c];
        }
        return score;
    }
};

const int8 CentralControlv2Heuristic::ctr2_col_weight[MAX_N] = {-40, -10, 0, 0, 0, 0, -10, -40};

// DefensiveMixed + MovableMaterial + SafeAdvancement

class DynamicWeightedHeuristic : public Heuristic {
public:
    int w_material;
    int w_advancement;
    int w_movable;
    int w_protect;
    int w_threat;
    int w_threat_and_protect;
    DynamicWeightedHeuristic(int wm=1000, int wa=10, int wmv=20, int wp=30, int wt=200, int wtp=30)
        : w_material(wm), w_advancement(wa), w_movable(wmv), w_protect(wp), w_threat(wt), w_threat_and_protect(wtp) {}
    
    const char* name() {
        char* buf = new char[64];
        sprintf(buf, "DW-%d-%d-%d-%d-%d-%d", w_material, w_advancement, w_movable, w_protect, w_threat, w_threat_and_protect);
        return buf;
    }

    int estimate(const BitBoard& b) {
        int score = 0;
        U64 p1 = b.get_p(1), p2 = b.get_p(2);
        U64 tmp = p1;
        while (tmp) {
            int8 idx = lowbit_id(tmp);
            lowbit_pop(tmp);
            int8 r = get_row(idx), c = get_col(idx);
            bool threat = false, protect = false;
            if (r - 1 >= 0) {
                if (c > 0 && (p1 & get_bit(r - 1, c - 1))) protect = true;
                if (c < MAX_N - 1 && (p1 & get_bit(r - 1, c + 1))) protect = true;
            }
            if (r < MAX_M - 1) {
                if (c > 0 && (p2 & get_bit(r + 1, c - 1))) threat = true;
                if (c < MAX_N - 1 && (p2 & get_bit(r + 1, c + 1))) threat = true;
            }
            score += w_material;
            score += w_advancement * MY_SQ[r];
            if (threat && !protect) score -= w_threat;
            else if (threat && protect) score -= w_threat_and_protect;
            else if (!threat && protect) score += w_protect;
        }
        tmp = p2;
        while (tmp) {
            int8 idx = lowbit_id(tmp);
            lowbit_pop(tmp);
            int8 r = get_row(idx), c = get_col(idx);
            bool threat = false, protect = false;
            if (r < MAX_M - 1) {
                if (c > 0 && (p2 & get_bit(r + 1, c - 1))) protect = true;
                if (c < MAX_N - 1 && (p2 & get_bit(r + 1, c + 1))) protect = true;
            }
            if (r - 1 >= 0) {
                if (c > 0 && (p1 & get_bit(r - 1, c - 1))) threat = true;
                if (c < MAX_N - 1 && (p1 & get_bit(r - 1, c + 1))) threat = true;
            }
            score -= w_material;
            score -= w_advancement * MY_SQ[MAX_M - 1 - r];
            if (threat && !protect) score += w_threat;
            else if (threat && protect) score += w_threat_and_protect;
            else if (!threat && protect) score -= w_protect;
        }
        U64 all = p1 | p2;
        U64 p1_st = (p1 << MAX_N) & ~all;
        U64 p1_dl = ((p1 & DL_MASK) << (MAX_N - 1)) & ~p1;
        U64 p1_dr = ((p1 & DR_MASK) << (MAX_N + 1)) & ~p1;
        score += w_movable * popcount(p1_st | p1_dl | p1_dr);
        U64 p2_st = (p2 >> MAX_N) & ~all;
        U64 p2_dl = ((p2 & DL_MASK) >> (MAX_N + 1)) & ~p2;
        U64 p2_dr = ((p2 & DR_MASK) >> (MAX_N - 1)) & ~p2;
        score -= w_movable * popcount(p2_st | p2_dl | p2_dr);
        return score;
    }
};

// search - ab negamax

int abNegamax(const BitBoard& b, U64 hash, int8 depth, int8 maxDepth, int8 player, Heuristic* h, int alpha, int beta) {
    static int node_count = 0;
    if (g_time_limit_ms > 0 && (++node_count & 8191) == 0) { // check every 8192 nodes
        auto now = chrono::steady_clock::now();
        if (chrono::duration<double, milli>(now - g_start_time).count() > g_time_limit_ms) {
            g_time_out = true;
        }
    }
    if (g_time_out) return 0;

    int8 winner = check_win(b);
    if (winner != 0) {
        if (winner == player) return MY_POS_INF + depth;
        else return MY_NEG_INF - depth;
    }
    
    if (depth == maxDepth) {
        return h->evaluate(b, player);
    }

    TTEntry *tt_entry = tt_get(hash);
    Move tt_best_move;
    if (tt_entry && tt_entry->valid) {
        tt_best_move = tt_entry->best_move;
        if (tt_entry->depth >= (maxDepth - depth)) {
            if (tt_entry->flag == EXACT) return tt_entry->score;
            else if (tt_entry->flag == LOWERBOUND && tt_entry->score >= beta) return tt_entry->score;
            else if (tt_entry->flag == UPPERBOUND && tt_entry->score <= alpha) return tt_entry->score;
        }
    }

    vector<Move> moves = get_all_moves(b, player, false, depth, &tt_best_move);
    if (moves.empty()) {
        return MY_NEG_INF - depth; 
    }

    Move best_move = moves[0];
    int best_score = MY_NEG_INF;
    TTFlag flag = UPPERBOUND;

    for (const Move& mv : moves) {
        BitBoard new_b = apply_move(b, mv);
        int8 dst_piece = b.get(mv.dst_r, mv.dst_c);
        U64 new_hash = incremental_hash(hash, mv.src_r, mv.src_c, player, mv.dst_r, mv.dst_c, dst_piece);
        
        int current_alpha = max(alpha, best_score);
        
        int score = -abNegamax(new_b, new_hash, depth + 1, maxDepth, 3 - player, h, -beta, -current_alpha);

        if (g_time_out) return 0;

        if (score > best_score) {
            best_score = score;
            best_move = mv;
            
            if (best_score >= beta) { // beta cutoff
                flag = LOWERBOUND;
                killer_update(depth, mv);
                break; 
            }
        }
    }

    if (!g_time_out) {
        if (best_score > alpha && best_score < beta) {
            flag = EXACT;
        }
        tt_update(hash, maxDepth - depth, flag, best_score, best_move);
    }
    
    return best_score;
}


Move abNegamax_solver(const BitBoard& b, int8 maxDepth, int8 player, Heuristic* h) {
    init_zobrist();
    tt_clear();
    killer_clear();
    
    g_time_out = false;
    g_time_limit_ms = 0; 

    U64 hash = get_hash(b, player);
    vector<Move> moves = get_all_moves(b, player);
    if (moves.empty()) return Move();

    Move best_move = moves[0];
    int alpha = MY_NEG_INF;
    int beta = MY_POS_INF;

    for (const Move& mv : moves) {
        BitBoard new_b = apply_move(b, mv);
        int8 dst_piece = b.get(mv.dst_r, mv.dst_c);
        U64 new_hash = incremental_hash(hash, mv.src_r, mv.src_c, player, mv.dst_r, mv.dst_c, dst_piece);
        int score = -abNegamax(new_b, new_hash, 1, maxDepth, 3 - player, h, -beta, -alpha);
        if (score > alpha) {
            alpha = score;
            best_move = mv;
        }
    }
    return best_move;
}

Move iterativeDeepening_solver(const BitBoard& b, int8 maxDepth, int8 player, Heuristic* h, double time_limit_ms = 0) {
    init_zobrist();
    tt_clear(); 
    killer_clear();

    vector<Move> moves = get_all_moves(b, player);
    if (moves.empty()) return Move();
    if (moves.size() == 1) return moves[0];

    g_time_out = false;
    g_start_time = chrono::steady_clock::now();
    g_time_limit_ms = time_limit_ms;

    Move best_move = moves[0];
    U64 hash = get_hash(b, player);

    for (int8 depth = 1; depth <= maxDepth; depth++) {
        int alpha = MY_NEG_INF;
        int beta = MY_POS_INF;
        Move current_best_move_in_depth = best_move;
        int current_best_score = MY_NEG_INF;

        // move best TT to front
        for (int i = 0; i < moves.size(); i++) {
            if (moves[i] == best_move) {
                swap(moves[0], moves[i]);
                break;
            }
        }

        bool complete_depth = true;

        for (const Move& mv : moves) {
            BitBoard new_b = apply_move(b, mv);
            int8 dst_piece = b.get(mv.dst_r, mv.dst_c);
            U64 new_hash = incremental_hash(hash, mv.src_r, mv.src_c, player, mv.dst_r, mv.dst_c, dst_piece);
            
            int score = -abNegamax(new_b, new_hash, 1, depth, 3 - player, h, -beta, -max(alpha, current_best_score));
            
            if (g_time_out) {
                complete_depth = false;
                break;
            }

            if (score > current_best_score) {
                current_best_score = score;
                current_best_move_in_depth = mv;
                
                if (score > alpha) alpha = score;
            }
        }

        if (complete_depth) {
            best_move = current_best_move_in_depth;
            if (current_best_score >= MY_NEAR_INF) break;
        } else {
            break; 
        }
    }
    return best_move;
}

// search - MCTS

struct MCTSNode {
    int8 player;
    int visits;
    int wins;
    BitBoard board;
    Move prev_move;
    MCTSNode* parent;
    vector<MCTSNode*> children;
    vector<Move> to_expand;

    constexpr static double K = 1.414; // exploration constant

    MCTSNode(BitBoard b, int8 p, Move mv, MCTSNode* parent) : player(p), visits(0), wins(0), board(b), prev_move(mv), parent(parent) {
        to_expand = get_all_moves(board, player);
    }

    ~MCTSNode() {
        for (MCTSNode* child : children) {
            delete child;
        }
    }

    bool is_terminal() {
        return check_win(board) != 0;
    }

    bool fully_expanded() {
        return to_expand.empty();
    }

    MCTSNode* expand() {
        int idx = global_rng() % to_expand.size();
        Move mv = to_expand[idx];
        to_expand.erase(to_expand.begin() + idx);

        BitBoard new_b = apply_move(board, mv);
        MCTSNode* child = new MCTSNode(new_b, 3 - player, mv, this);
        children.push_back(child);
        return child;
    }

    MCTSNode* best_child() {
        MCTSNode* best_node = nullptr;
        double best_uct = -1e9;
        double log_visits = log((double)visits);
        for (MCTSNode* child : children) {
            // u_i = w_i / n_i + K * sqrt(log(N) / n_i)
            double uct = (double)child->wins / (child->visits + 1e-9) + K * sqrt(log_visits / (child->visits + 1e-9));
            if (uct > best_uct) {
                best_uct = uct;
                best_node = child;
            }
        }
        return best_node;
    }
};

const int8 MCTS_TRUNCATE = 200;

int8 mcts_heuristic_playout(BitBoard b, int8 player, Heuristic* h = nullptr, double epsilon = 0.7) {
    for (int8 steps = 0; steps < MCTS_TRUNCATE; steps++) {
        int8 winner = check_win(b);
        if (winner != 0) return winner;

        vector<Move> moves = get_all_moves(b, player);
        if (moves.empty()) return 3 - player;

        Move mv;
        if (h == nullptr || (double)global_rng() / global_rng.max() < epsilon) { // random move
            mv = moves[global_rng() % moves.size()];
        } else { // heuristic move
            int best_score = MY_NEG_INF;
            for (const Move& m : moves) {
                BitBoard new_b = apply_move(b, m);
                int score = h->evaluate(new_b, player);
                if (score > best_score) {
                    best_score = score;
                    mv = m;
                }
            }
        }
        b = apply_move(b, mv);
        player = 3 - player;
    }
    // if truncated, decide winner by material
    int8 p1_count = popcount(b.get_p(1));
    int8 p2_count = popcount(b.get_p(2));
    return (p1_count >= p2_count) ? 1 : 2;
}

Move mcts_solver(const BitBoard& b, int8 player, Heuristic* h = nullptr, int max_iters = 100000, double time_limit_ms = 0) {
    MCTSNode * root = new MCTSNode(b, player, Move(), nullptr);

    auto start_time = chrono::steady_clock::now();

    for (int iter = 0; iter < max_iters; iter++) {
        // Selection
        MCTSNode* node = root;
        while (!node->is_terminal() && node->fully_expanded()) {
            node = node->best_child();
        }

        // Expansion
        if (!node->is_terminal() && !node->fully_expanded()) {
            node = node->expand();
        }

        // Simulation
        int8 winner = mcts_heuristic_playout(node->board, node->player, h);

        // Backpropagation
        MCTSNode* back_node = node;
        while (back_node) {
            back_node->visits++;
            if (back_node->player != winner) {
                back_node->wins++; // win is counted for the parent player, which is the opposite of the current player
            }
            back_node = back_node->parent;
        }

        if (time_limit_ms > 0 && iter > 0 && (iter & 127) == 0) { // check time every 128 iters
            auto now = chrono::steady_clock::now();
            double elapsed = chrono::duration<double, milli>(now - start_time).count();
            if (elapsed > time_limit_ms) break;
        }
    }

    MCTSNode* best_child = nullptr;
    int best_visits = -1;
    for (MCTSNode* child : root->children) {
        if (child->visits > best_visits) {
            best_visits = child->visits;
            best_child = child;
        }
    }
    // return best_child ? best_child->prev_move : get_all_moves(b, player, true)[0]; // fallback but should not happen
    Move best_move = best_child ? best_child->prev_move : get_all_moves(b, player, true)[0];
    delete root; // will recursively delete all nodes
    return best_move;
}

#ifndef LOCAL

// submission
// time limit = 2000ms
// memory limit = 10000KB

void play_games(int step) {
    Position playerA_positions[MAX_PIECES];
    Position playerB_positions[MAX_PIECES];
    int playerA_count, playerB_count;
    Board board;

    read_ckbd(step - 1, playerA_positions, &playerA_count,
              playerB_positions, &playerB_count, board);

    BitBoard b(board);
    // int player = (step % 2 != 0) ? 1 : 2;
    int8 player = (step & 1) ? 1 : 2;

    // Heuristic* h = new MaterialHeuristic();
    // Heuristic* h = new MovableMaterialHeuristic();
    // Heuristic* h = new DynamicWeightedHeuristic(1000, 39, 32, 30, 139, 92);
    Heuristic* h = new DynamicWeightedHeuristic(1000, 50, 0, 0, 139, 92);
    Move best_move = iterativeDeepening_solver(b, 10, player, h, 1500);

    save_decision(best_move.src_r, best_move.src_c, best_move.dst_r, best_move.dst_c);
    delete h;
}

#endif

// tournament

#ifdef LOCAL

// const BitBoard START_BOARD = []() {
//     BitBoard b;
//     for (int c = 0; c < MAX_N; c++) {
//         b.set(0, c, 1);
//         b.set(1, c, 1);
//         b.set(6, c, 2);
//         b.set(7, c, 2);
//     }
//     return b;
// }();

const BitBoard START_BOARD = BitBoard(
    0x000000000000FFFFULL,
    0xFFFF000000000000ULL
);

enum SolverType { AB, ID, MCTS };

struct SolverProfile {
    const char* name;
    SolverType type;
    Heuristic* h;
    int max_depth;
    int max_iters;
    double time_limit_ms;

    Move get_move(const BitBoard& b, int8 player) const {
        if (type == AB) {
            return abNegamax_solver(b, max_depth, player, h);
        } else if (type == ID) {
            return iterativeDeepening_solver(b, max_depth, player, h, time_limit_ms);
        } else if (type == MCTS) {
            return mcts_solver(b, player, h, max_iters, time_limit_ms);
        }
        return Move();
    }
};

struct MatchResult {
    int8 winner;
    int steps;
    double p1_avg_time;
    double p2_avg_time;
};

MatchResult play(const SolverProfile& p1, const SolverProfile &p2, int max_rounds = 100) {
    BitBoard b = START_BOARD;
    double p1_time = 0, p2_time = 0;

    for (int round = 1; round <= max_rounds; round++) {
        for (int8 player = 1; player <= 2; player++) {
            int8 winner = check_win(b);
            if (winner != 0) {
                p1_time /= (player == 1) ? (round - 1) : round;
                p2_time /= (round - 1);
                return {winner, round, p1_time, p2_time};
            }

            auto start_time = chrono::steady_clock::now();
            Move mv = (player == 1) ? p1.get_move(b, player) : p2.get_move(b, player);
            auto now = chrono::steady_clock::now();
            double elapsed = chrono::duration<double, milli>(now - start_time).count();
            if (player == 1) p1_time += elapsed;
            else p2_time += elapsed;

            b = apply_move(b, mv);
        }
    }

    int8 p1_count = popcount(b.get_p(1));
    int8 p2_count = popcount(b.get_p(2));
    int8 winner = (p1_count >= p2_count) ? 1 : 2;
    p1_time /= max_rounds;
    p2_time /= max_rounds;
    return {winner, max_rounds, p1_time, p2_time};
}

void random_search(int n_strategies, int trials) {
    // randomly search for best parameters for DynamicWeightedHeuristic
    vector<SolverProfile> opponents = {
        {"Material", AB, new MaterialHeuristic(), 6, 0, 0},
        {"DefensiveMixed", ID, new DefensiveMixedHeuristic(), 100, 0, 1500},
        {"MovableMaterial", ID, new MovableMaterialHeuristic(), 100, 0, 1500},
        {"SafeAdvancement", ID, new SafeAdvancementHeuristic(), 100, 0, 1500}
    };
    vector<SolverProfile> candidates;
    for (int i = 0; i < n_strategies; i++) {
        // base (1000, 10, 20, 30, 200, 30)
        int wm = 1000;
        int wa = global_rng() % 40;
        int wmv = global_rng() % 60;
        int wp = global_rng() % 80;
        int wt = global_rng() % 300;
        int wtp = global_rng() % 100;
        Heuristic* h = new DynamicWeightedHeuristic(wm, wa, wmv, wp, wt, wtp);
        candidates.push_back({h->name(), ID, h, 100, 0, 1500});
    }

    FILE* output_csv = fopen("random_search_summary.csv", "w");
    fprintf(output_csv, "Candidate,Material,DefensiveMixed,MovableMaterial,SafeAdvancement,TotalWins\n");
    vector<vector<int>> results(candidates.size(), vector<int>(opponents.size(), 0));
    for (int i = 0; i < candidates.size(); i++) {
        for (int j = 0; j < opponents.size(); j++) {
            int p1_wins = 0, p2_wins = 0;
            for (int t = 0; t < trials; t++) {
                MatchResult res = play(candidates[i], opponents[j], 100);
                if (res.winner == 1) p1_wins++;
                else p2_wins++;
            }
            results[i][j] = p1_wins;
            printf("Candidate %s vs Opponent %s: %d/%d\n", candidates[i].name, opponents[j].name, p1_wins, trials);
        }
    }
    // sort by total wins
    vector<pair<int, int>> candidate_wins; // (wins, index)
    for (int i = 0; i < candidates.size(); i++) {
        int total_wins = 0;
        for (int j = 0; j < opponents.size(); j++) {
            total_wins += results[i][j];
        }
        candidate_wins.push_back({total_wins, i});
    }
    sort(candidate_wins.rbegin(), candidate_wins.rend()); // win descending

    for (const auto& [total_wins, idx] : candidate_wins) {
        fprintf(output_csv, "%s", candidates[idx].name);
        for (int j = 0; j < opponents.size(); j++) {
            fprintf(output_csv, ",%d", results[idx][j]);
        }
        fprintf(output_csv, ",%d\n", total_wins);
    }
    fclose(output_csv);
}

void SingleTargetMatch(const SolverProfile& target, const vector<SolverProfile>& opponents, int trials) {
    FILE* output_csv = fopen("single_target_match_summary.csv", "w");
    fprintf(output_csv, "Opponent,TargetWins,OpponentWins\n");
    for (const SolverProfile& opponent : opponents) {
        int target_wins = 0, opponent_wins = 0;
        for (int t = 0; t < trials; t++) {
            MatchResult res = play(target, opponent, 100);
            if (res.winner == 1) target_wins++;
            else opponent_wins++;
        }
        fprintf(output_csv, "%s,%d,%d\n", opponent.name, target_wins, opponent_wins);
        printf("Target %s vs Opponent %s: %d/%d\n", target.name, opponent.name, target_wins, trials);
    }
    fclose(output_csv);
}

int main() {
    // printf("%llx\n", START_BOARD.get_p(1));
    // printf("%llx\n", START_BOARD.get_p(2));

    init_zobrist();

    // random_search(60, 10);
    // return 0;

    Heuristic* rnd = new RandomHeuristic();
    Heuristic* mat = new MaterialHeuristic();
    Heuristic* adv = new AdvancementHeuristic();
    Heuristic* atk = new AttackHeuristic();
    Heuristic* def = new DefenseHeuristic();

    Heuristic* agg_m = new AggresiveMixedHeuristic();
    Heuristic* def_m = new DefensiveMixedHeuristic();
    Heuristic* mov = new MovabilityHeuristic();
    Heuristic* mov_mat = new MovableMaterialHeuristic();
    Heuristic* ctr = new CentralControlHeuristic();
    Heuristic* adv_safe = new SafeAdvancementHeuristic();

    Heuristic* pass = new PassedPawnHeuristic();
    Heuristic* contact = new ContactExchangeHeuristic();
    Heuristic* w_mov = new WeightedMovabilityHeuristic();
    Heuristic* ctr2 = new CentralControlv2Heuristic();

    vector<SolverProfile> solvers = {
        // {"AB4-Random", AB, rnd, 4, 0, 0},
        // {"AB4-Material", AB, mat, 4, 0, 0},
        // {"AB4-Advancement", AB, adv, 4, 0, 0},
        // {"AB4-Attack", AB, atk, 4, 0, 0},
        // {"AB4-Defense", AB, def, 4, 0, 0},
        // {"AB4-AggressiveMixed", AB, agg_m, 4, 0, 0},
        // {"AB4-DefensiveMixed", AB, def_m, 4, 0, 0},
        // {"AB4-Movability", AB, mov, 4, 0, 0},
        // {"AB4-MovableMaterial", AB, mov_mat, 4, 0, 0},
        // {"AB4-CentralControl", AB, ctr, 4, 0, 0},
        // {"AB4-SafeAdvancement", AB, adv_safe, 4, 0, 0},

        // {"AB5-Material", AB, mat, 5, 0, 0},
        {"AB6-Material", AB, mat, 6, 0, 0},
        // {"AB7-Material", AB, mat, 7, 0, 0},
        // {"AB8-Material", AB, mat, 8, 0, 0},

        // {"ID-Material", ID, mat, 100, 0, 1500},
        {"ID-Advancement", ID, adv, 100, 0, 1500},
        // {"ID-Attack", ID, atk, 100, 0, 1500},
        // {"ID-Defense", ID, def, 100, 0, 1500},
        {"ID-AggresiveMixed", ID, agg_m, 100, 0, 1500},
        {"ID-DefensiveMixed", ID, def_m, 100, 0, 1500},
        // {"ID-Movability", ID, mov, 100, 0, 1500},
        {"ID-MovableMaterial", ID, mov_mat, 100, 0, 1500},
        // {"ID-CentralControl", ID, ctr, 100, 0, 1500},
        {"ID-SafeAdvancement", ID, adv_safe, 100, 0, 1500},
        // {"ID-PassedPawn", ID, pass, 100, 0, 1500},
        // {"ID-ContactExchange", ID, contact, 100, 0, 1500},
        // {"ID-WeightedMovability", ID, w_mov, 100, 0, 1500},
        // {"ID-CentralControlv2", ID, ctr2, 100, 0, 1500}

        // {"MCTS-Material", MCTS, mat, 0, 1000000, 1500},
        // {"MCTS-Advancement", MCTS, adv, 0, 1000000, 1500},
        // {"MCTS-AggresiveMixed", MCTS, agg_m, 0, 1000000, 1500},
        // {"MCTS-DefensiveMixed", MCTS, def_m, 0, 1000000, 1500},
        // {"MCTS-MovableMaterial", MCTS, mov_mat, 0, 1000000, 1500},
        // {"MCTS-SafeAdvancement", MCTS, adv_safe, 0, 1000000, 1500}
        {"DW-1000-33-48-69-266-74", ID, new DynamicWeightedHeuristic(1000, 33, 48, 69, 266, 74), 100, 0, 1500},
        {"DW-1000-1-22-0-193-59", ID, new DynamicWeightedHeuristic(1000, 1, 22, 0, 193, 59), 100, 0, 1500},
        {"DW-1000-39-32-30-139-92", ID, new DynamicWeightedHeuristic(1000, 39, 32, 30, 139, 92), 100, 0, 1500},
        {"DW-1000-38-1-25-192-81", ID, new DynamicWeightedHeuristic(1000, 38, 1, 25, 192, 81), 100, 0, 1500},
        {"DW-1000-18-17-31-23-22", ID, new DynamicWeightedHeuristic(1000, 18, 17, 31, 23, 22), 100, 0, 1500}
    };

    vector<SolverProfile> opponents = {
        {"DW-1000-39-0-30-139-92", ID, new DynamicWeightedHeuristic(1000, 39, 0, 30, 139, 92), 100, 0, 1500},
        {"ID-DefensiveMixed", ID, def_m, 100, 0, 1500},
        {"ID-MovableMaterial", ID, mov_mat, 100, 0, 1500},
        {"ID-SafeAdvancement", ID, adv_safe, 100, 0, 1500}
    };

    SolverProfile target = {"DW-1000-39-0-0-139-92", ID, new DynamicWeightedHeuristic(1000, 39, 0, 0, 139, 92), 100, 0, 1500};

    SingleTargetMatch(target, opponents, 10);
    return 0;


    // vector<SolverProfile> solvers = {
    //     {"DW-1000-39-0-0-139-92", ID, new DynamicWeightedHeuristic(1000, 39, 0, 0, 139, 92), 100, 0, 1500},
    //     // {"DW-1000-39-32-30-139-92", ID, new DynamicWeightedHeuristic(1000, 39, 32, 30, 139, 92), 100, 0, 1500},
    //     // {"DW-1000-39-32-30-139-0", ID, new DynamicWeightedHeuristic(1000, 39, 32, 30, 139, 0), 100, 0, 1500},
    //     // {"DW-1000-39-32-30-0-92", ID, new DynamicWeightedHeuristic(1000, 39, 32, 30, 0, 92), 100, 0, 1500},
    //     // {"DW-1000-39-32-0-139-92", ID, new DynamicWeightedHeuristic(1000, 39, 32, 0, 139, 92), 100, 0, 1500},
    //     {"DW-1000-39-0-30-139-92", ID, new DynamicWeightedHeuristic(1000, 39, 0, 30, 139, 92), 100, 0, 1500},
    //     // {"DW-1000-0-32-30-139-92", ID, new DynamicWeightedHeuristic(1000, 0, 32, 30, 139, 92), 100, 0, 1500},
    //     // {"DW-1000-39-32-30-139-32", ID, new DynamicWeightedHeuristic(1000, 39, 32, 30, 139, 32), 100, 0, 1500},
    //     {"ID-DefensiveMixed", ID, def_m, 100, 0, 1500},
    //     {"ID-MovableMaterial", ID, mov_mat, 100, 0, 1500},
    //     {"ID-SafeAdvancement", ID, adv_safe, 100, 0, 1500}
    // };

    int n = solvers.size();
    int trials = 10;

    vector<vector<int>> wins_p1(n, vector<int>(n, 0));
    vector<vector<int>> wins_p2(n, vector<int>(n, 0));
    vector<vector<double>> time_p1(n, vector<double>(n, 0));
    vector<vector<double>> time_p2(n, vector<double>(n, 0));

    // FILE* output_csv = fopen("match_results.csv", "r+");
    FILE* output_csv = fopen("match_results.csv", "w");

    // char line[256];
    // while (fgets(line, sizeof(line), output_csv)) {
    //     // printf("Loaded from CSV: %s", line);
    //     char name1[64], name2[64];
    //     int w1, w2, t;
    //     double time1, time2;
    //     if (sscanf(line, "%63[^,],%63[^,],%d,%d,%d,%lf,%lf", name1, name2, &w1, &w2, &t, &time1, &time2) == 7) {
    //         int idx1 = -1, idx2 = -1;
    //         for (int i = 0; i < n; i++) {
    //             if (strcmp(solvers[i].name, name1) == 0) idx1 = i;
    //             if (strcmp(solvers[i].name, name2) == 0) idx2 = i;
    //         }
    //         if (idx1 != -1 && idx2 != -1) {
    //             wins_p1[idx1][idx2] = w1;
    //             wins_p2[idx2][idx1] = w2;
    //             time_p1[idx1][idx2] = time1;
    //             time_p2[idx2][idx1] = time2;
    //             printf("Loaded from CSV: %s vs %s - P1 wins: %d, P2 wins: %d, Time: %.1fms vs %.1fms\n", name1, name2, w1, w2, time1, time2);
    //         }
    //     }
    // }

    puts("Match results:");

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i == j) continue;
            if (wins_p1[i][j] > 0 || wins_p2[j][i] > 0) {
                continue;
            }
            printf("%-28s vs %-28s: ", solvers[i].name, solvers[j].name);
            for (int t = 0; t < trials; t++) {
                MatchResult res = play(solvers[i], solvers[j]);
                if (res.winner == 1) wins_p1[i][j]++;
                else wins_p2[j][i]++;
                time_p1[i][j] += res.p1_avg_time;
                time_p2[j][i] += res.p2_avg_time;
            }
            time_p1[i][j] /= trials;
            time_p2[j][i] /= trials;
            printf("P1 wins: %2d/%2d, Time: %.1fms vs %.1fms\n", wins_p1[i][j], trials, time_p1[i][j], time_p2[j][i]);
            fprintf(output_csv, "%s,%s,%d,%d,%d,%.1f,%.1f\n", solvers[i].name, solvers[j].name, wins_p1[i][j], wins_p2[j][i], trials, time_p1[i][j], time_p2[j][i]);
        }
    }
    fclose(output_csv);

    puts("\nFinal rankings:");
    printf(" # | %-28s | Wins | AvgTime\n", "Solver");
    for (int i = 0; i < 50; i++) putchar('-');
    putchar('\n');
    
    vector<int> ranking(n);
    for (int i = 0; i < n; i++) ranking[i] = i;

    vector<int> total_wins(n);
    vector<double> total_time(n);

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            total_wins[i] += wins_p1[i][j] + wins_p2[i][j];
            total_time[i] += time_p1[i][j] + time_p2[i][j];
        }
    }

    sort(ranking.begin(), ranking.end(), [&](int p1, int p2) {
        return total_wins[p1] > total_wins[p2];
    }); // more wins ranks higher

    for (int idx = 0; idx < n; idx++) {
        int i = ranking[idx];
        printf("%02d | %-28s | %4d | %7.1f\n", idx + 1, solvers[i].name, total_wins[i], total_time[i] / (n - 1) / 2);
    }
}

#endif