#!/usr/bin/env python3
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Sequence, Tuple

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Circle, Rectangle


BOARD_SIZE = 7
P1 = 1
P2 = 2

COLOR_P1 = "#2C7BE5"
COLOR_P2 = "#F2C94C"
COLOR_GRID_A = "#F8F6F1"
COLOR_GRID_B = "#EAE6DC"
COLOR_CLONE = "#2A9D8F"
COLOR_JUMP = "#E76F51"


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


@dataclass(frozen=True)
class CellBox:
    rc: Tuple[int, int]
    color: str
    lw: float = 2.2


def in_bounds(r: int, c: int) -> bool:
    return 0 <= r < BOARD_SIZE and 0 <= c < BOARD_SIZE


def build_board(p1_pieces: Iterable[Tuple[int, int]], p2_pieces: Iterable[Tuple[int, int]]) -> Dict[Tuple[int, int], int]:
    board: Dict[Tuple[int, int], int] = {}
    for rc in p1_pieces:
        board[rc] = P1
    for rc in p2_pieces:
        board[rc] = P2
    return board


def draw_piece(ax: plt.Axes, r: int, c: int, player: int, alpha: float = 1.0) -> None:
    face = COLOR_P1 if player == P1 else COLOR_P2
    ax.add_patch(Circle((c, r), radius=0.34, facecolor=face, edgecolor="#1F1F1F", linewidth=1.0, alpha=alpha, zorder=4))


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

    draw_piece(ax, 0, 0, P1, alpha=0.22)
    draw_piece(ax, 6, 6, P1, alpha=0.22)
    draw_piece(ax, 0, 6, P2, alpha=0.22)
    draw_piece(ax, 6, 0, P2, alpha=0.22)


def clone_and_jump_targets(board: Dict[Tuple[int, int], int], r: int, c: int) -> Tuple[List[Tuple[int, int]], List[Tuple[int, int]]]:
    clones: List[Tuple[int, int]] = []
    jumps: List[Tuple[int, int]] = []

    for dr in (-1, 0, 1):
        for dc in (-1, 0, 1):
            if dr == 0 and dc == 0:
                continue
            nr = r + dr
            nc = c + dc
            if in_bounds(nr, nc) and (nr, nc) not in board:
                clones.append((nr, nc))

    for dr, dc in ((-2, 0), (2, 0), (0, -2), (0, 2)):
        nr = r + dr
        nc = c + dc
        if in_bounds(nr, nc) and (nr, nc) not in board:
            jumps.append((nr, nc))

    return clones, jumps


def render_scene(
    out_path: Path,
    title: str,
    subtitle: str,
    board: Dict[Tuple[int, int], int],
    anchors: Sequence[Tuple[int, int]],
    anchor_labels: Sequence[str],
    arrow_notes: Sequence[ArrowNote],
    text_notes: Sequence[TextNote],
    boxes: Sequence[CellBox],
) -> None:
    fig, ax = plt.subplots(figsize=(6.35, 5.85), constrained_layout=True)
    draw_board_base(ax)

    for (r, c), player in board.items():
        draw_piece(ax, r, c, player)

    for box in boxes:
        r, c = box.rc
        ax.add_patch(Rectangle((c - 0.48, r - 0.48), 0.96, 0.96, fill=False, linewidth=box.lw, edgecolor=box.color, zorder=6))

    anchor_colors = ["#E63946", "#9D4EDD", "#2A9D8F", "#F77F00"]
    for i, (r, c) in enumerate(anchors):
        color = anchor_colors[i % len(anchor_colors)]
        ax.add_patch(Rectangle((c - 0.48, r - 0.48), 0.96, 0.96, fill=False, linewidth=2.6, edgecolor=color, zorder=7))
        label = anchor_labels[i] if i < len(anchor_labels) else f"X{i + 1}"
        ax.text(c, r, label, color="white", fontsize=11, weight="bold", ha="center", va="center", zorder=10)

    for a in arrow_notes:
        src_r, src_c = a.src
        dst_r, dst_c = a.dst
        ax.annotate(
            "",
            xy=(dst_c, dst_r),
            xytext=(src_c, src_r),
            arrowprops=dict(arrowstyle="->", lw=2.0, color=a.color),
            zorder=8,
            alpha=0.8,
        )

    for t in text_notes:
        r, c = t.rc
        ax.text(
            c + t.dx,
            r + t.dy,
            t.text,
            fontsize=9.7,
            color=t.color,
            bbox=dict(facecolor="white", edgecolor="none", alpha=0.87, boxstyle="round,pad=0.22"),
            zorder=9,
        )

    ax.set_xlim(-0.55, BOARD_SIZE - 0.45)
    ax.set_ylim(BOARD_SIZE - 0.45, -1.02)
    ax.set_aspect("equal")
    ax.set_xticks(range(BOARD_SIZE))
    ax.set_yticks(range(BOARD_SIZE))
    ax.tick_params(labelsize=10)
    ax.set_xlabel("col", fontsize=11)
    ax.set_ylabel("row", fontsize=11)
    ax.set_title(title, fontsize=16, weight="bold")
    ax.text(-0.48, -0.9, subtitle, fontsize=10, color="#333333", ha="left", va="top")

    out_path.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(out_path, dpi=130)
    plt.close(fig)


def generate_all_diagrams(output_dir: Path) -> None:
    render_scene(
        output_dir / "material.png",
        "Material",
        "f = count(P1 pieces)",
        build_board([(4, 3), (5, 1)], [(2, 5), (5, 5)]),
        [(4, 3), (5, 1)],
        ["A", "B"],
        [],
        [TextNote((4, 3), "A: +1", "#111", 0.45, -0.2), TextNote((5, 1), "B: +1", "#111", 0.45, 0.2)],
        [],
    )

    b = build_board([(3, 3), (1, 2)], [(2, 2), (4, 4), (0, 2)])
    c1, j1 = clone_and_jump_targets(b, 3, 3)
    c2, j2 = clone_and_jump_targets(b, 1, 2)
    render_scene(
        output_dir / "mobility.png",
        "Mobility",
        "f = |CloneMoves| + |JumpMoves|",
        b,
        [(3, 3), (1, 2)],
        ["A", "B"],
        [ArrowNote((3, 3), c1[0], "clone", COLOR_CLONE), ArrowNote((3, 3), j1[0], "jump", COLOR_JUMP), ArrowNote((1, 2), c2[0], "clone", COLOR_CLONE), ArrowNote((1, 2), j2[0], "jump", COLOR_JUMP)],
        [TextNote((3, 3), f"A: {len(c1)}+{len(j1)}={len(c1)+len(j1)}", "#111", 0.52, -0.18), TextNote((1, 2), f"B: {len(c2)}+{len(j2)}={len(c2)+len(j2)}", "#111", 0.52, 0.2), TextNote((0, 0), "green=clone, red=jump", "#444", 0.2, -0.2)],
        [],
    )

    render_scene(
        output_dir / "center_control.png",
        "Center Control",
        "f = sum(50 - 4*((r-3)^2 + (c-3)^2))",
        build_board([(5, 2), (2, 4)], [(1, 6)]),
        [(5, 2), (2, 4)],
        ["A", "B"],
        [ArrowNote((5, 2), (3, 3), "to center", "#6A4C93"), ArrowNote((2, 4), (3, 3), "to center", "#6A4C93")],
        [TextNote((5, 2), "A: 50-4*5=30", "#111", 0.55, -0.2), TextNote((2, 4), "B: 50-4*2=42", "#111", 0.55, 0.2)],
        [CellBox((3, 3), "#6A4C93")],
    )

    render_scene(
        output_dir / "infection_pressure.png",
        "Infection Pressure",
        "f = 12*enemyAdj + 4*friendAdj",
        build_board([(3, 3), (2, 1), (1, 1)], [(2, 3), (4, 3), (2, 2), (1, 2), (3, 1)]),
        [(3, 3), (2, 1)],
        ["A", "B"],
        [ArrowNote((3, 3), (2, 3), "enemy", "#B23A48"), ArrowNote((3, 3), (4, 3), "enemy", "#B23A48"), ArrowNote((2, 1), (2, 2), "enemy", "#B23A48"), ArrowNote((2, 1), (3, 1), "enemy", "#B23A48"), ArrowNote((2, 1), (1, 1), "friend", "#2A9D8F")],
        [TextNote((3, 3), "A: 12*2+4*0=24", "#111", 0.56, -0.2), TextNote((2, 1), "B: 12*2+4*1=28", "#111", 0.56, 0.2)],
        [],
    )

    b = build_board([(3, 2), (1, 5)], [(5, 2), (2, 4)])
    c1, j1 = clone_and_jump_targets(b, 3, 2)
    c2, j2 = clone_and_jump_targets(b, 1, 5)
    render_scene(
        output_dir / "expansion.png",
        "Expansion",
        "f = 6*|CloneReach| + 3*|JumpReach|",
        b,
        [(3, 2), (1, 5)],
        ["A", "B"],
        [ArrowNote((3, 2), c1[0], "clone", COLOR_CLONE), ArrowNote((3, 2), j1[0], "jump", COLOR_JUMP), ArrowNote((1, 5), c2[0], "clone", COLOR_CLONE), ArrowNote((1, 5), j2[0], "jump", COLOR_JUMP)],
        [TextNote((3, 2), f"A: 6*{len(c1)}+3*{len(j1)}={6*len(c1)+3*len(j1)}", "#111", 0.55, -0.2), TextNote((1, 5), f"B: 6*{len(c2)}+3*{len(j2)}={6*len(c2)+3*len(j2)}", "#111", -2.6, 0.2), TextNote((0, 0), "green=clone, red=jump", "#444", 0.2, -0.2)],
        [],
    )

    render_scene(
        output_dir / "safety.png",
        "Safety",
        "f = 6*friendAdj - 8*enemyAdj",
        build_board([(4, 3), (3, 1), (1, 3), (1, 2)], [(5, 2), (5, 4), (1, 4)]),
        [(4, 3), (1, 3)],
        ["A", "B"],
        [ArrowNote((4, 3), (5, 2), "enemy", "#B23A48"), ArrowNote((4, 3), (5, 4), "enemy", "#B23A48"), ArrowNote((4, 3), (3, 1), "friend", "#2A9D8F"), ArrowNote((1, 3), (1, 2), "friend", "#2A9D8F"), ArrowNote((1, 3), (1, 4), "enemy", "#B23A48")],
        [TextNote((4, 3), "A: 6*1-8*2=-10", "#111", 0.55, -0.2), TextNote((1, 3), "B: 6*1-8*1=-2", "#111", 0.55, 0.2)],
        [],
    )

    render_scene(
        output_dir / "influence.png",
        "Influence",
        "f(E) = 9*(P1_adj(E) - P2_adj(E))",
        build_board([(3, 2), (4, 4), (1, 1)], [(2, 4), (3, 4), (4, 2), (2, 2)]),
        [],
        [],
        [ArrowNote((3, 2), (3, 3), "+P1", "#2A9D8F"), ArrowNote((4, 4), (3, 3), "+P1", "#2A9D8F"), ArrowNote((2, 4), (3, 3), "-P2", "#B23A48"), ArrowNote((3, 4), (3, 3), "-P2", "#B23A48"), ArrowNote((1, 1), (1, 2), "+P1", "#2A9D8F"), ArrowNote((2, 2), (1, 2), "-P2", "#B23A48")],
        [TextNote((3, 3), "E1: 9*(2-2)=0", "#111", 0.24, -0.36), TextNote((1, 2), "E2: 9*(1-1)=0", "#111", 0.24, 0.35)],
        [CellBox((3, 3), "#6A4C93"), CellBox((1, 2), "#6A4C93")],
    )

    render_scene(
        output_dir / "frontier.png",
        "Frontier",
        "f = 6*frontierCount (early)",
        build_board([(3, 1), (6, 6), (2, 5)], [(2, 6), (5, 5)]),
        [(3, 1), (6, 6)],
        ["A", "B"],
        [ArrowNote((3, 1), (3, 2), "adj empty", "#2A9D8F"), ArrowNote((6, 6), (5, 6), "adj empty", "#2A9D8F")],
        [TextNote((3, 1), "A: frontier=1 => +6", "#111", 0.55, -0.2), TextNote((6, 6), "B: frontier=1 => +6", "#111", -2.15, -0.2)],
        [],
    )

    render_scene(
        output_dir / "position_weight.png",
        "Position Weight",
        "f = w[r][c]",
        build_board([(0, 0), (1, 1), (3, 3)], [(6, 6)]),
        [(0, 0), (1, 1)],
        ["A", "B"],
        [],
        [TextNote((0, 0), "A: w[0,0]=90", "#111", 0.62, -0.16), TextNote((1, 1), "B: w[1,1]=-50", "#111", 0.62, 0.18)],
        [CellBox((0, 0), "#1D3557"), CellBox((1, 1), "#E63946")],
    )

    render_scene(
        output_dir / "potential_conversion.png",
        "Potential Conversion",
        "f = 25*enemyAdj",
        build_board([(3, 3), (1, 2)], [(2, 3), (3, 2), (4, 4), (0, 2)]),
        [(3, 3), (1, 2)],
        ["A", "B"],
        [ArrowNote((3, 3), (2, 3), "enemy", "#B23A48"), ArrowNote((3, 3), (3, 2), "enemy", "#B23A48"), ArrowNote((3, 3), (4, 4), "enemy", "#B23A48"), ArrowNote((1, 2), (0, 2), "enemy", "#B23A48")],
        [TextNote((3, 3), "A: 25*3=75", "#111", 0.55, -0.2), TextNote((1, 2), "B: 25*1=25", "#111", 0.55, 0.2)],
        [],
    )

    render_scene(
        output_dir / "control_area.png",
        "Control Area",
        "f = sum(32*I[d=1] + 16*I[d=2])",
        build_board([(3, 3), (1, 1)], [(6, 6)]),
        [(3, 3), (1, 1)],
        ["A", "B"],
        [ArrowNote((3, 3), (3, 4), "d1", "#6A4C93"), ArrowNote((3, 3), (1, 3), "d2", "#3A86FF"), ArrowNote((1, 1), (1, 2), "d1", "#6A4C93"), ArrowNote((1, 1), (3, 1), "d2", "#3A86FF")],
        [TextNote((3, 3), "A: 32+16=48", "#111", 0.55, -0.2), TextNote((1, 1), "B: 32+16=48", "#111", 0.55, 0.2)],
        [CellBox((3, 4), "#6A4C93"), CellBox((1, 3), "#3A86FF"), CellBox((1, 2), "#6A4C93"), CellBox((3, 1), "#3A86FF")],
    )

    render_scene(
        output_dir / "aggression.png",
        "Aggression",
        "f = (12 - nearestDist)*8",
        build_board([(5, 1), (2, 2)], [(2, 4), (5, 6), (1, 1)]),
        [(5, 1), (2, 2)],
        ["A", "B"],
        [ArrowNote((5, 1), (2, 4), "nearest", "#E63946"), ArrowNote((2, 2), (1, 1), "nearest", "#E63946")],
        [TextNote((5, 1), "A: d=6 => (12-6)*8=48", "#111", 0.6, -0.2), TextNote((2, 2), "B: d=2 => (12-2)*8=80", "#111", 0.6, 0.2)],
        [],
    )

    b = build_board([(3, 3), (1, 4)], [(2, 3), (2, 4), (5, 4)])
    c1, j1 = clone_and_jump_targets(b, 3, 3)
    c2, j2 = clone_and_jump_targets(b, 1, 4)
    render_scene(
        output_dir / "hybrid.png",
        "Hybrid",
        "f = (40 - 3*d^2) + 10*enemyAdj + 4*cloneReach",
        b,
        [(3, 3), (1, 4)],
        ["A", "B"],
        [ArrowNote((3, 3), c1[0], "clone", COLOR_CLONE), ArrowNote((3, 3), j1[0], "jump", COLOR_JUMP), ArrowNote((1, 4), c2[0], "clone", COLOR_CLONE), ArrowNote((1, 4), j2[0], "jump", COLOR_JUMP)],
        [TextNote((3, 3), f"A: (40-0)+10*2+4*{len(c1)}={40+20+4*len(c1)}", "#111", 0.56, -0.2), TextNote((1, 4), f"B: (40-15)+10*1+4*{len(c2)}={25+10+4*len(c2)}", "#111", -2.65, 0.2), TextNote((0, 0), "green=clone, red=jump", "#444", 0.2, -0.2)],
        [CellBox((3, 3), "#6A4C93"), CellBox((1, 4), "#6A4C93")],
    )


def main() -> None:
    out_dir = Path("ataxx/images/heuristic")
    generate_all_diagrams(out_dir)
    print(f"Saved diagrams to: {out_dir.resolve()}")


if __name__ == "__main__":
    main()
