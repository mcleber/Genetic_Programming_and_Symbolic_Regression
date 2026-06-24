"""
plot_evolution.py
Robust annotated evolution chart for Genetic Programming runs.
All landmarks are derived strictly from CSV data (no heuristics).
"""

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# ─────────────────────────────────────────────────────────────
# LOAD DATA
# ─────────────────────────────────────────────────────────────
df = pd.read_csv("evolution_fitness.csv")

GEN  = df["generation"].values
BEST = df["best_fitness"].values
AVG  = df["average_fitness"].values
SIZE = df["average_size"].values
N    = len(GEN)

# ─────────────────────────────────────────────────────────────
# LANDMARKS (DATA-DRIVEN)
# ─────────────────────────────────────────────────────────────

best_val = BEST.max()
converge_gen = int(np.where(BEST == best_val)[0][-1])

worst_avg_gen = int(np.argmin(AVG))

window_size = 10
threshold = 0.05 * (AVG.max() - AVG.min() + 1e-9)

stable_start = N - 1
for g in range(0, N - window_size):
    window = AVG[g:g + window_size]
    if np.std(window) < threshold:
        stable_start = g
        break

peak_size_gen = int(np.argmax(SIZE))
peak_size_val = float(SIZE[peak_size_gen])

min_solution = 5

# ─────────────────────────────────────────────────────────────
# COLORS
# ─────────────────────────────────────────────────────────────
C_BEST   = "#1a56db"
C_AVG    = "#60a5fa"
C_IDEAL  = "#16a34a"
C_BLOAT  = "#dc2626"
C_PURPLE = "#7c3aed"
C_ORANGE = "#f97316"
C_GREY   = "#374151"

# ─────────────────────────────────────────────────────────────
# FIGURE
# ─────────────────────────────────────────────────────────────
fig, (ax1, ax2) = plt.subplots(
    2, 1, figsize=(13, 9), sharex=True,
    gridspec_kw={"hspace": 0.08}
)

fig.patch.set_facecolor("#f8fafc")
for ax in (ax1, ax2):
    ax.set_facecolor("#f8fafc")

# ─────────────────────────────────────────────────────────────
# FITNESS PANEL
# ─────────────────────────────────────────────────────────────
ax1.plot(GEN, BEST, color=C_BEST, lw=2.2, label="Best fitness")
ax1.plot(GEN, AVG,  color=C_AVG,  lw=1.6, ls="--", label="Average fitness")
ax1.axhline(0, color=C_IDEAL, ls=":", lw=1.8, label="Ideal (0 error)")

ax1.set_ylabel("Fitness")
ax1.set_title("Fitness Evolution per Generation")
ax1.grid(True, alpha=0.2)
ax1.legend()

# ─────────────────────────────────────────────────────────────
# ✅ CORREÇÃO PRINCIPAL: ESCALA DO EIXO Y
# ─────────────────────────────────────────────────────────────
y_min = min(BEST.min(), AVG.min())
y_max = max(BEST.max(), AVG.max())

# margem inteligente (evita BEST colado no topo)
padding = 0.15 * (y_max - y_min + 1e-9)

ax1.set_ylim(
    y_min - padding,
    y_max + padding * 1.5   # mais espaço no topo (importante!)
)

# ─────────────────────────────────────────────────────────────
# BLOAT PANEL
# ─────────────────────────────────────────────────────────────
ax2.plot(GEN, SIZE, color=C_BLOAT, lw=2.0, label="Average tree size")
ax2.axhline(min_solution, color=C_PURPLE, ls=":", lw=1.6,
            label="Minimal solution (5 nodes)")

ax2.set_xlabel("Generation")
ax2.set_ylabel("Nodes")
ax2.set_title("Tree Growth (Bloat Monitoring)")
ax2.grid(True, alpha=0.2)
ax2.legend()

ax2.set_ylim(0, SIZE.max() * 1.15)

# ─────────────────────────────────────────────────────────────
# PHASE LINES
# ─────────────────────────────────────────────────────────────
phase_lines = [converge_gen, worst_avg_gen, stable_start]

for ax in (ax1, ax2):
    for g in phase_lines:
        ax.axvline(g, color="#94a3b8", ls="--", lw=1, alpha=0.5)

# ─────────────────────────────────────────────────────────────
# SAFE INDEXING
# ─────────────────────────────────────────────────────────────
def clamp(i):
    return max(0, min(N - 1, i))

# 1) convergence
ax1.annotate(
    f"Convergence ~ gen {converge_gen}",
    xy=(converge_gen, BEST[converge_gen]),
    xytext=(clamp(converge_gen + 5), BEST.max() * 0.6),
    arrowprops=dict(arrowstyle="->", color=C_BEST),
    bbox=dict(fc="white", ec=C_BEST),
)

# 2) worst avg
ax1.annotate(
    f"Worst avg gen {worst_avg_gen}",
    xy=(worst_avg_gen, AVG[worst_avg_gen]),
    xytext=(clamp(worst_avg_gen + 5), AVG.min() * 0.8),
    arrowprops=dict(arrowstyle="->", color=C_ORANGE),
    bbox=dict(fc="white", ec=C_ORANGE),
)

# 3) stability
ax1.annotate(
    f"Stabilization ~ {stable_start}",
    xy=(stable_start, AVG[stable_start]),
    xytext=(clamp(stable_start + 5), AVG.mean()),
    arrowprops=dict(arrowstyle="->", color=C_IDEAL),
    bbox=dict(fc="white", ec=C_IDEAL),
)

# ─────────────────────────────────────────────────────────────
# BLOAT ANNOTATION
# ─────────────────────────────────────────────────────────────
ax2.annotate(
    f"Peak bloat: {peak_size_val:.0f} nodes",
    xy=(peak_size_gen, peak_size_val),
    xytext=(clamp(peak_size_gen - 10), peak_size_val * 0.9),
    arrowprops=dict(arrowstyle="->", color=C_ORANGE),
    bbox=dict(fc="white", ec=C_ORANGE),
)

# ─────────────────────────────────────────────────────────────
# FINAL
# ─────────────────────────────────────────────────────────────
plt.tight_layout()
#plt.savefig("evolution_annotated.png", dpi=150, bbox_inches="tight")
plt.show()