#!/usr/bin/env python3
"""Create report-ready diagrams for breakthrough heuristics in minimax.cpp.

The script draws an 8x8 board and explains how player1 piece contributions are
computed for each heuristic via highlighted anchor pieces, arrows and labels.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Sequence, Tuple

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Circle, Rectangle


BOARD_SIZE = 8
P1 = 1
P2 = 2

COLOR_P1 = "#2C7BE5"  # blue
COLOR_P2 = "#F2C94C"  # yellow
COLOR_GRID_A = "#F8F6F1"
COLOR_GRID_B = "#EAE6DC"


@dataclass(frozen=True)
class ArrowNote:
    src: Tuple[int, int]
    dst: Tuple[int, int]
    text: str
    color: str = "#444444"


@dataclass(frozen=True)
class TextNote:
    rc: Tuple[int, int]
    text: str
    color: str = "#222222"
    dx: float = 0.0
    dy: float = 0.0


def build_board(p1_pieces: Iterable[Tuple[int, int]], p2_pieces: Iterable[Tuple[int, int]]) -> Dict[Tuple[int, int], int]:
    board: Dict[Tuple[int, int], int] = {}
    for rc in p1_pieces:
        board[rc] = P1
    for rc in p2_pieces:
        board[rc] = P2
    return board


def draw_board_base(ax: plt.Axes) -> None:
    for r in range(BOARD_SIZE):
        for c in range(BOARD_SIZE):
            color = COLOR_GRID_A if (r + c) % 2 == 0 else COLOR_GRID_B
            ax.add_patch(Rectangle((c - 0.5, r - 0.5), 1.0, 1.0, facecolor=color, edgecolor="none", zorder=0))

    for k in range(BOARD_SIZE + 1):
        x = k - 0.5
        y = k - 0.5
        ax.plot([x, x], [-0.5, BOARD_SIZE - 0.5], color="#999999", linewidth=1.0, zorder=1)
        ax.plot([-0.5, BOARD_SIZE - 0.5], [y, y], color="#999999", linewidth=1.0, zorder=1)

    # Semi-transparent baseline rows to indicate each side's home line.
    for c in range(BOARD_SIZE):
        ax.add_patch(Circle((c, 0), radius=0.34, facecolor=COLOR_P1, edgecolor="none", alpha=0.20, zorder=2))
        ax.add_patch(Circle((c, BOARD_SIZE - 1), radius=0.34, facecolor=COLOR_P2, edgecolor="none", alpha=0.20, zorder=2))

    # ax.text(BOARD_SIZE - 0.55, -0.82, "row 0: P1 base", fontsize=11, ha="right", va="center", color="#555555")
    ax.text(-1.72, -0.32, "P1 base", fontsize=11, ha="left", va="center", color="#555555")
    ax.text(-1.72, BOARD_SIZE - 0.68, "P2 base", fontsize=11, ha="left", va="center", color="#555555")
    # ax.text(
    #     BOARD_SIZE - 0.55,
    #     BOARD_SIZE - 0.18,
    #     "row 7: P2 base",
    #     fontsize=11,
    #     ha="right",
    #     va="center",
    #     color="#555555",
    # )


def draw_piece(ax: plt.Axes, r: int, c: int, player: int, alpha: float = 1.0) -> None:
    if player == P1:
        face = COLOR_P1
    else:
        face = COLOR_P2
    ax.add_patch(Circle((c, r), radius=0.34, facecolor=face, edgecolor="#1F1F1F", linewidth=1.0, alpha=alpha, zorder=4))


def render_scene(
    out_path: Path,
    title: str,
    subtitle: str,
    board: Dict[Tuple[int, int], int],
    anchors: Sequence[Tuple[int, int]],
    anchor_labels: Sequence[str],
    arrow_notes: Sequence[ArrowNote],
    text_notes: Sequence[TextNote],
) -> None:
    fig, ax = plt.subplots(figsize=(6.4, 5.8), constrained_layout=True)
    draw_board_base(ax)

    for (r, c), player in board.items():
        draw_piece(ax, r, c, player)

    anchor_colors = ["#E63946", "#9D4EDD", "#2A9D8F", "#F77F00"]
    for i, (r, c) in enumerate(anchors):
        color = anchor_colors[i % len(anchor_colors)]
        ax.add_patch(Rectangle((c - 0.48, r - 0.48), 0.96, 0.96, fill=False, linewidth=2.5, edgecolor=color, zorder=6))
        label = anchor_labels[i] if i < len(anchor_labels) else f"X{i + 1}"
        ax.text(c, r, label, color="white", fontsize=12, weight="bold", ha="center", va="center", zorder=10)

    for a in arrow_notes:
        src_r, src_c = a.src
        dst_r, dst_c = a.dst
        ax.annotate(
            "",
            xy=(dst_c, dst_r),
            xytext=(src_c, src_r),
            arrowprops=dict(arrowstyle="->", lw=2.0, color=a.color),
            zorder=8,
            alpha=0.75,
        )
        mx = (src_c + dst_c) * 0.5
        my = (src_r + dst_r) * 0.5
        ax.text(
            mx + 0.05,
            my - 0.08,
            a.text,
            fontsize=11,
            color=a.color,
            bbox=dict(facecolor="white", edgecolor="none", alpha=0.75, boxstyle="round,pad=0.2"),
            zorder=9,
        )

    for t in text_notes:
        r, c = t.rc
        ax.text(
            c + t.dx,
            r + t.dy,
            t.text,
            fontsize=11,
            color=t.color,
            bbox=dict(facecolor="white", edgecolor="none", alpha=0.86, boxstyle="round,pad=0.25"),
            zorder=9,
        )

    ax.set_xlim(-0.55, BOARD_SIZE - 0.45)
    ax.set_ylim(BOARD_SIZE - 0.45, -1.05)
    ax.set_aspect("equal")
    ax.set_xticks(range(BOARD_SIZE))
    ax.set_yticks(range(BOARD_SIZE))
    ax.tick_params(labelsize=11)
    ax.set_xlabel("col", fontsize=12)
    ax.set_ylabel("row", fontsize=12)
    ax.set_title(title, fontsize=17, weight="bold")
    # make some space for subtitle
    ax.text(-0.5, -0.92, subtitle, fontsize=11, color="#333333", ha="left", va="top")
    out_path.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(out_path, dpi=120)
    plt.close(fig)


def p1_legal_moves_for_piece(board: Dict[Tuple[int, int], int], r: int, c: int) -> List[Tuple[int, int]]:
    if board.get((r, c)) != P1:
        return []

    moves: List[Tuple[int, int]] = []
    next_r = r + 1
    if next_r >= BOARD_SIZE:
        return moves

    if board.get((next_r, c), 0) == 0:
        moves.append((next_r, c))

    left_c = c - 1
    if left_c >= 0 and board.get((next_r, left_c), 0) != P1:
        moves.append((next_r, left_c))

    right_c = c + 1
    if right_c < BOARD_SIZE and board.get((next_r, right_c), 0) != P1:
        moves.append((next_r, right_c))

    return moves


def generate_all_diagrams(output_dir: Path) -> None:
    # Material
    anchors = [(4, 3), (6, 6)]
    board = build_board(p1_pieces=anchors, p2_pieces=[(5, 2), (6, 0)])
    render_scene(
        out_path=output_dir / "material.png",
        title="Material",
        subtitle="= 1 * (number of pieces)",
        board=board,
        anchors=anchors,
        anchor_labels=["+1", "+1"],
        arrow_notes=[],
        text_notes=[],
    )

    # Advancement
    anchors = [(4, 3), (6, 6)]
    board = build_board(p1_pieces=anchors, p2_pieces=[(5, 2), (6, 0)])
    arrow_notes = [
        ArrowNote(anchors[0], (0, anchors[0][1]), "r=4", "#E63946"),
        ArrowNote(anchors[1], (0, anchors[1][1]), "r=6", "#9D4EDD"),
    ]
    render_scene(
        out_path=output_dir / "advancement.png",
        title="Advancement",
        subtitle="= r (distance from base row)",
        board=board,
        anchors=anchors,
        anchor_labels=["+4", "+6"],
        arrow_notes=arrow_notes,
        text_notes=[],
    )

    # Attack
    anchors = [(3, 3), (5, 1), (5, 3)]
    board = build_board(p1_pieces=anchors, p2_pieces=[(4, 2), (4, 4), (5, 2), (6, 0)])
    render_scene(
        out_path=output_dir / "attack.png",
        title="Attack",
        subtitle="= 1 * (each possible capture)",
        board=board,
        anchors=anchors,
        anchor_labels=["+2", "+1", "0"],
        arrow_notes=[
            ArrowNote(anchors[0], (4, 2), "+1", "#B23A48"),
            ArrowNote(anchors[0], (4, 4), "+1", "#B23A48"),
            ArrowNote(anchors[1], (6, 0), "+1", "#9D4EDD"),
        ],
        text_notes=[],
    )

    # Defense
    anchors = [(4, 3), (6, 4), (6, 6)]
    board = build_board(p1_pieces=[*anchors, (3, 2), (3, 4), (5, 3)], p2_pieces=[(5, 2), (6, 0)])
    render_scene(
        out_path=output_dir / "defense.png",
        title="Defense",
        subtitle="= 1 * (each defended piece)",
        board=board,
        anchors=anchors,
        anchor_labels=["+2", "+1", "0"],
        arrow_notes=[
            ArrowNote(anchors[0], (3, 2), "+1", "#B23A48"),
            ArrowNote(anchors[0], (3, 4), "+1", "#B23A48"),
            ArrowNote(anchors[1], (5, 3), "+1", "#9D4EDD"),
        ],
        text_notes=[],
    )

    # AggresiveMixed (spelling follows source code)
    anchors = [(5, 2), (6, 5)]
    score_a = 100 + anchors[0][0] * anchors[0][0] + (500 if anchors[0][0] >= 6 else 0)
    score_b = 100 + anchors[1][0] * anchors[1][0] + (500 if anchors[1][0] >= 6 else 0)
    board = build_board(p1_pieces=anchors, p2_pieces=[(6, 0)])
    render_scene(
        out_path=output_dir / "aggressive_mixed.png",
        # title="AggresiveMixed = 100 (Material) + r^2 (Advancement) + 500 (if on second-to-last row)",
        title="Aggressive Mixed",
        subtitle="= 100 (Mat) + r^2 (Adv) + 500 (if on second-to-last row)",
        board=board,
        anchors=anchors,
        anchor_labels=[f"{score_a}", f"{score_b}"],
        arrow_notes=[
            ArrowNote(anchors[0], (0, anchors[0][1]), "r=5,100+5^2", "#E63946"),
            ArrowNote(anchors[1], (0, anchors[1][1]), "r=6,100+6^2+500", "#9D4EDD"),
        ],
        text_notes=[],
    )

    # DefensiveMixed
    anchors = [(4, 2), (5, 5)]
    board = build_board(p1_pieces=[anchors[0], anchors[1], (3, 1), (3, 3), (4, 6)], p2_pieces=[(5, 2), (6, 0)])
    def_count_1 = int(board.get((3, 1), 0) == P1) + int(board.get((3, 3), 0) == P1)
    def_count_2 = int(board.get((4, 4), 0) == P1) + int(board.get((4, 6), 0) == P1)
    score_1 = 100 + anchors[0][0] * anchors[0][0] + def_count_1
    score_2 = 100 + anchors[1][0] * anchors[1][0] + def_count_2
    render_scene(
        out_path=output_dir / "defensive_mixed.png",
        # title="DefensiveMixed = 100 (Material) + r^2 (Advancement) + 1 * (defended pieces)",
        title="Defensive Mixed",
        subtitle="= 100 (Mat) + r^2 (Adv) + 1 * (defended pieces)",
        board=board,
        anchors=anchors,
        anchor_labels=[f"{score_1}", f"{score_2}"],
        arrow_notes=[
            ArrowNote(anchors[0], (3, 1), "+1", "#B23A48"),
            ArrowNote(anchors[0], (3, 3), "+1", "#B23A48"),
            ArrowNote(anchors[0], (0, anchors[0][1]), "r=4,100+4^2", "#B23A48"),
            ArrowNote(anchors[1], (4, 6), "+1", "#9D4EDD"),
            ArrowNote(anchors[1], (0, anchors[1][1]), "r=5,100+5^2", "#9D4EDD"),
        ],
        text_notes=[],
    )

    # Movability (local piece contribution)
    anchors = [(4, 3), (5, 6)]
    board = build_board(p1_pieces=[anchors[0], anchors[1], (6, 5)], p2_pieces=[(5, 2), (6, 7)])
    m1 = p1_legal_moves_for_piece(board, anchors[0][0], anchors[0][1])
    m2 = p1_legal_moves_for_piece(board, anchors[1][0], anchors[1][1])
    score_1 = 100 + 2 * len(m1)
    score_2 = 100 + 2 * len(m2)
    arrow_notes: List[ArrowNote] = []
    for dst in m1:
        arrow_notes.append(ArrowNote(anchors[0], dst, "+2", "#B23A48"))
    for dst in m2:
        arrow_notes.append(ArrowNote(anchors[1], dst, "+2", "#9D4EDD"))
    render_scene(
        out_path=output_dir / "movable_material.png",
        # title="Movability = 100 (Material) + 2 * (legal moves)",
        title="Movable Material",
        subtitle="= 100 (Material) + 2 * (legal moves)",
        board=board,
        anchors=anchors,
        anchor_labels=[f"{score_1}", f"{score_2}"],
        arrow_notes=arrow_notes,
        text_notes=[],
    )

    anchors = [(4, 3), (5, 6)]
    board = build_board(p1_pieces=anchors, p2_pieces=[(5, 2), (6, 0)])
    ctrl_weights = [0, 1, 2, 3, 3, 2, 1, 0]
    score_1 = 100 + anchors[0][0] * anchors[0][0] + ctrl_weights[anchors[0][1]] * 10
    score_2 = 100 + anchors[1][0] * anchors[1][0] + ctrl_weights[anchors[1][1]] * 10
    render_scene(
        out_path=output_dir / "central_control.png",
        title="Central Control",
        subtitle="= 100 (Mat) + r^2 (Adv) + 10 * (col weight)",
        board=board,
        anchors=anchors,
        anchor_labels=[f"{score_1}", f"{score_2}"],
        arrow_notes=[
            ArrowNote(anchors[0], (0, anchors[0][1]), "r=4,100+4^2", "#E63946"),
            ArrowNote(anchors[0], (anchors[0][0], 0), "c=3, +30", "#E63946"),
            ArrowNote(anchors[1], (0, anchors[1][1]), "r=5,100+5^2", "#9D4EDD"),
            ArrowNote(anchors[1], (anchors[1][0], 7), "c=6, +10", "#9D4EDD"),
        ],
        text_notes=[],
    )

    anchors = [(4, 3), (5, 6), (4, 0)]
    board = build_board(p1_pieces=[*anchors, (4, 5), (3, 1)], p2_pieces=[(5, 2), (6, 0), (6, 7)])
    score_1 = 100 + anchors[0][0] * anchors[0][0] - 30
    score_2 = 100 + anchors[1][0] * anchors[1][0] - 5
    score_3 = 100 + anchors[2][0] * anchors[2][0]
    render_scene(
        out_path=output_dir / "safe_advancement.png",
        title="Safe Advancement",
        subtitle="= 100 (Mat) + r^2 (Adv) - 30 or 5 (threatened)",
        board=board,
        anchors=anchors,
        anchor_labels=[f"{score_1}", f"{score_2}", f"{score_3}"],
        arrow_notes=[
            ArrowNote(anchors[0], (5, 2), "threatened", "#B23A48"),
            ArrowNote(anchors[0], (2, 3), "not protected (-30)", "#B23A48"),
            ArrowNote(anchors[1], (6, 7), "threatened", "#9D4EDD"),
            ArrowNote(anchors[1], (4, 5), "protected (-5)", "#9D4EDD"),
        ],
        text_notes=[],
    )


def main() -> None:
    out_dir = Path("heuristic")
    generate_all_diagrams(out_dir)
    print(f"Saved diagrams to: {out_dir.resolve()}")


if __name__ == "__main__":
    main()
