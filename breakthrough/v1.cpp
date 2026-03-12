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

using namespace std;

#ifndef MAX_M
#define MAX_M 8
#define MAX_N 8
#define MAX_PIECES 16
#endif

const int MY_NEG_INF = (int)-1e9;
const int MY_POS_INF = (int)1e9;

// Move structure
struct Move {
    int src_r, src_c;
    int dst_r, dst_c;

    Move(int sr, int sc, int dr, int dc) : src_r(sr), src_c(sc), dst_r(dr), dst_c(dc) {}
};

struct BoardState {
    int grid[MAX_M][MAX_N];

    BoardState() {
        memset(grid, 0, sizeof(grid));
    }

    BoardState(int arr[MAX_M][MAX_N]) {
        memcpy(grid, arr, sizeof(grid));
    }
};

// Check for winner (1 or 2), or 0 if no winner
int check_win(BoardState b) {
    for (int c = 0; c < MAX_N; c++) {
        if (b.grid[0][c] == 2) return 2;
        if (b.grid[MAX_M - 1][c] == 1) return 1;
    }
    int p1_pieces = 0, p2_pieces = 0;
    for (int r = 0; r < MAX_M; r++) {
        for (int c = 0; c < MAX_N; c++) {
            if (b.grid[r][c] == 1) p1_pieces++;
            else if (b.grid[r][c] == 2) p2_pieces++;
        }
    }
    if (p1_pieces == 0) return 2;
    if (p2_pieces == 0) return 1;
    return 0;
}

BoardState apply_move(BoardState b, Move mv) {
    int piece = b.grid[mv.src_r][mv.src_c];
    b.grid[mv.src_r][mv.src_c] = 0;
    b.grid[mv.dst_r][mv.dst_c] = piece;
    return b;
}

vector<Move> get_all_moves(BoardState b, int player) {
    vector<Move> moves;
    int direction = player == 1 ? 1 : -1;

    for (int i = 0; i < MAX_M; i++) {
        // pieces closer to opponent's side are considered first to improve pruning
        int r = player == 1 ? (MAX_M - 1 - i) : i;
        int next_r = r + direction;
        if (next_r < 0 || next_r >= MAX_M) continue;
        for (int c = 0; c < MAX_N; c++) {
            if (b.grid[r][c] == player) {
                // Forward move
                if (b.grid[next_r][c] == 0) {
                    moves.emplace_back(r, c, next_r, c);
                }
                // Diagonal move or capture
                if (c > 0 && b.grid[next_r][c - 1] != player) {
                    moves.emplace_back(r, c, next_r, c - 1);
                }
                if (c < MAX_N - 1 && b.grid[next_r][c + 1] != player) {
                    moves.emplace_back(r, c, next_r, c + 1);
                }
            }
        }
    }
    
    return moves;
}

class Heuristic {
public:
    virtual const char* name() = 0;
    virtual int estimate(BoardState b) = 0;
    int evaluate(BoardState b, int player) {
        int winner = check_win(b);
        if (winner == player) return MY_POS_INF;
        else if (winner != 0) return MY_NEG_INF;
        return estimate(b) * (player == 1 ? 1 : -1);
    }
};

class RandomHeuristic : public Heuristic {
public:
    const char* name() { return "Random"; }
    int estimate(BoardState b) {
        return rand() % 201 - 100; // [-100, 100]
    }
};

class MaterialHeuristic : public Heuristic {
public:
    const char* name() { return "Material"; }
    int estimate(BoardState b) {
        int score = 0;
        for (int r = 0; r < MAX_M; r++) {
            for (int c = 0; c < MAX_N; c++) {
                if (b.grid[r][c] == 1) score++;
                else if (b.grid[r][c] == 2) score--;
            }
        }
        return score;
    }
};

class AdvancementHeuristic : public Heuristic {
public:
    const char* name() { return "Advancement"; }
    int estimate(BoardState b) {
        int score = 0;
        for (int r = 0; r < MAX_M; r++) {
            for (int c = 0; c < MAX_N; c++) {
                if (b.grid[r][c] == 1) score += r;
                else if (b.grid[r][c] == 2) score -= (MAX_M - 1 - r);
            }
        }
        return score;
    }
};

class AttackHeuristic : public Heuristic {
public:
    const char* name() { return "Attack"; }
    int estimate(BoardState b) {
        int score = 0;
        for (int r = 0; r < MAX_M; r++) {
            for (int c = 0; c < MAX_N; c++) {
                if (b.grid[r][c] == 1) {
                    if (r + 1 < MAX_M) {
                        if (c > 0 && b.grid[r + 1][c - 1] == 2) score++;
                        if (c < MAX_N - 1 && b.grid[r + 1][c + 1] == 2) score++;
                    }
                } else if (b.grid[r][c] == 2) {
                    if (r - 1 >= 0) {
                        if (c > 0 && b.grid[r - 1][c - 1] == 1) score--;
                        if (c < MAX_N - 1 && b.grid[r - 1][c + 1] == 1) score--;
                    }
                }
            }
        }
        return score;
    }
};

class DefenseHeuristic : public Heuristic {
public:
    const char* name() { return "Defense"; }
    int estimate(BoardState b) {
        int score = 0;
        for (int r = 0; r < MAX_M; r++) {
            for (int c = 0; c < MAX_N; c++) {
                if (b.grid[r][c] == 1) {
                    if (r - 1 >= 0) {
                        if (c > 0 && b.grid[r - 1][c - 1] == 1) score++;
                        if (c < MAX_N - 1 && b.grid[r - 1][c + 1] == 1) score++;
                    }
                } else if (b.grid[r][c] == 2) {
                    if (r + 1 < MAX_M) {
                        if (c > 0 && b.grid[r + 1][c - 1] == 2) score--;
                        if (c < MAX_N - 1 && b.grid[r + 1][c + 1] == 2) score--;
                    }
                }
            }
        }
        return score;
    }
};

class AggresiveMixedHeuristic : public Heuristic {
public:
    const char* name() { return "AggresiveMixed"; }
    int estimate(BoardState b) {
        int score = 0;
        for (int r = 0; r < MAX_M; r++) {
            for (int c = 0; c < MAX_N; c++) {
                if (b.grid[r][c] == 1) {
                    score += 100;
                    score += r * r;
                    if (r >= MAX_M - 2) score += 500;
                } else if (b.grid[r][c] == 2) {
                    score -= 100;
                    score -= (MAX_M - 1 - r) * (MAX_M - 1 - r);
                    if (r <= 1) score -= 500; // heavily penalize opponent pieces close to winning
                }
            }
        }
        return score;
    }
};

class DefensiveMixedHeuristic : public Heuristic {
public:
    const char* name() { return "DefensiveMixed"; }
    int estimate(BoardState b) {
        int score = 0;
        for (int r = 0; r < MAX_M; r++) {
            for (int c = 0; c < MAX_N; c++) {
                if (b.grid[r][c] == 1) {
                    score += 100;
                    score += r * r;
                    if (r - 1 >= 0) {
                        if (c > 0 && b.grid[r - 1][c - 1] == 1) score++;
                        if (c < MAX_N - 1 && b.grid[r - 1][c + 1] == 1) score++;
                    }
                } else if (b.grid[r][c] == 2) {
                    score -= 100;
                    score -= (MAX_M - 1 - r) * (MAX_M - 1 - r);
                    if (r + 1 < MAX_M) {
                        if (c > 0 && b.grid[r + 1][c - 1] == 2) score--;
                        if (c < MAX_N - 1 && b.grid[r + 1][c + 1] == 2) score--;
                    }
                }
            }
        }
        return score;
    }
};

class MovabilityHeuristic : public Heuristic {
public:
    const char* name() { return "Movability"; }
    int estimate(BoardState b) {
        int score = 0;
        for (int r = 0; r < MAX_M; r++) {
            for (int c = 0; c < MAX_N; c++) {
                if (b.grid[r][c] == 1) score++;
                else if (b.grid[r][c] == 2) score--;
            }
        }
        score *= 100;

        vector<Move> p1_moves = get_all_moves(b, 1);
        vector<Move> p2_moves = get_all_moves(b, 2);
        score += (int)p1_moves.size() * 2;
        score -= (int)p2_moves.size() * 2;
        return score;
    }
};

int abNegamax(BoardState b, int depth, int maxDepth, int player, Heuristic* h, int alpha, int beta) {
    if (depth == maxDepth || check_win(b) != 0) {
        return h->evaluate(b, player);
    }

    vector<Move> moves = get_all_moves(b, player);
    if (moves.empty()) {
        return h->evaluate(b, player);
    }

    int bestScore = MY_NEG_INF;
    for (const Move& mv : moves) {
        BoardState new_b = apply_move(b, mv);
        int score = -abNegamax(new_b, depth + 1, maxDepth, 3 - player, h, -beta, -max(alpha, bestScore));
        bestScore = max(bestScore, score);
        if (bestScore >= beta) {
            break; // Beta cut-off
        }
    }
    return bestScore;
}

Move abNegamax_solver(BoardState b, int maxDepth, int player, Heuristic* h) {
    vector<Move> moves = get_all_moves(b, player);
    Move best_move = moves[0];
    int alpha = MY_NEG_INF;
    int beta = MY_POS_INF;

    for (const Move& mv : moves) {
        BoardState new_b = apply_move(b, mv);
        int score = -abNegamax(new_b, 1, maxDepth, 3 - player, h, -beta, -alpha);
        if (score > alpha) {
            alpha = score;
            best_move = mv;
        }
    }
    return best_move;
}

#ifndef LOCAL

void play_games(int step) {
    // Seed random
    srand(time(NULL) + step);

    // 1. Read BoardState state
    Position playerA_positions[MAX_PIECES];
    Position playerB_positions[MAX_PIECES];
    int playerA_count, playerB_count;
    Board board;

    read_ckbd(step - 1, playerA_positions, &playerA_count,
              playerB_positions, &playerB_count, board);

    BoardState b(board);

    // 2. Determine current player
    int player = (step % 2 != 0) ? 1 : 2;

    Heuristic* h = new MaterialHeuristic();
    
    Move choice = abNegamax_solver(b, 6, player, h);

    // 5. Save decision
    save_decision(choice.src_r, choice.src_c, choice.dst_r, choice.dst_c);
}

#endif

#ifdef LOCAL

int start_grid[MAX_M][MAX_N] = {
    {1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {2, 2, 2, 2, 2, 2, 2, 2},
    {2, 2, 2, 2, 2, 2, 2, 2}
};

BoardState start_BoardState(start_grid);

int play(Heuristic* p1, Heuristic* p2, int depth) {
    BoardState b = start_BoardState;
    int player = 1;

    while (true) {
        int winner = check_win(b);
        if (winner != 0) return winner;

        Heuristic* h = (player == 1) ? p1 : p2;
        Move mv = abNegamax_solver(b, depth, player, h);
        b = apply_move(b, mv);
        player = 3 - player;
    }

    return 0;
}

void tournament(int depth, int trials) {
    vector<Heuristic*> heuristics = {
        new RandomHeuristic(),
        new MaterialHeuristic(),
        new AdvancementHeuristic(),
        new AttackHeuristic(),
        new DefenseHeuristic(),
        new AggresiveMixedHeuristic(),
        new DefensiveMixedHeuristic(),
        new MovabilityHeuristic()
    };
    int n = heuristics.size();
    vector<vector<int>> results(n, vector<int>(n, 0));

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i == j) continue;
            // fprintf(stderr, "Testing %s vs %s... ", heuristics[i]->name(), heuristics[j]->name());
            for (int t = 0; t < trials; t++) {
                int winner = play(heuristics[i], heuristics[j], depth);
                if (winner == 1) results[i][j]++;
            }
            // fprintf(stderr, "%d/%d wins\n", results[i][j], trials);
        }
    }

    printf("Results:\n");
    for (int i = 0; i < n; i++) {
        printf("%15s\t", heuristics[i]->name());
        for (int j = 0; j < n; j++) {
            printf("%d\t", results[i][j]);
        }
        printf("\n");
    }

    vector<int> ranking(n);
    for (int i = 0; i < n; i++) ranking[i] = i;

    sort(ranking.begin(), ranking.end(), [&](int p1, int p2) {
        int score1 = 0, score2 = 0;
        for (int j = 0; j < n; j++) { // judge by total wins against others
            if (j != p1) score1 += results[p1][j];
            if (j != p2) score2 += results[p2][j];
        }
        return score1 > score2;
    });

    printf("Ranking:\n");
    for (int i = 0; i < n; i++) {
        printf("%d: %s\n", i + 1, heuristics[ranking[i]]->name());
    }
}

int main() {
    tournament(4, 20);
    return 0;
}

#endif
