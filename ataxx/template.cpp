#include "battle_base.h"
#include <array>
#include <cstdlib>
#include <ctime>
#include <vector>

using std::array;
using std::vector;

static inline bool in_bounds(int x, int y) {
    return x >= 0 && x < MAX_M && y >= 0 && y < MAX_N;
}

static inline void try_push_move(const Board board, vector<Decision> &moves,
                                 int from_x, int from_y, int to_x, int to_y) {
    if (!in_bounds(to_x, to_y)) {
        return;
    }
    if (board[to_x][to_y] != 0) {
        return;
    }
    moves.push_back({from_x, from_y, to_x, to_y});
}

static vector<Decision> collect_legal_moves(const Board board, int player) {
    vector<Decision> moves;
    static const array<array<int, 2>, 4> jumps = {{{-2, 0}, {2, 0}, {0, -2}, {0, 2}}};

    for (int x = 0; x < MAX_M; ++x) {
        for (int y = 0; y < MAX_N; ++y) {
            if (board[x][y] != player) {
                continue;
            }

            // Clone to any adjacent square.
            for (int dx = -1; dx <= 1; ++dx) {
                for (int dy = -1; dy <= 1; ++dy) {
                    if (dx == 0 && dy == 0) {
                        continue;
                    }
                    try_push_move(board, moves, x, y, x + dx, y + dy);
                }
            }

            // Jump exactly 2 squares orthogonally.
            for (const auto &jump : jumps) {
                try_push_move(board, moves, x, y, x + jump[0], y + jump[1]);
            }
        }
    }

    return moves;
}

void play_games(int step) {
    std::srand(static_cast<unsigned int>(std::time(nullptr)) + static_cast<unsigned int>(step));

    Board board;
    read_ckbd(step - 1, board);

    int player = (step % 2 != 0) ? 1 : 2;
    vector<Decision> legal_moves = collect_legal_moves(board, player);

    if (legal_moves.empty()) {
        save_decision(0, 0, 0, 0);
        return;
    }

    // Example policy: choose one legal move uniformly at random.
    const Decision &choice = legal_moves[std::rand() % legal_moves.size()];
    save_decision(choice.x1, choice.y1, choice.x2, choice.y2);
}
