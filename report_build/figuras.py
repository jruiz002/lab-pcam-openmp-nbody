"""Genera las figuras (PDF) y tablas (.tex) del reporte a partir de results.csv."""
import csv
import pathlib

import matplotlib

matplotlib.use("pdf")
import matplotlib.pyplot as plt

HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE.parent
GEN = HERE / "generado"
GEN.mkdir(exist_ok=True)

rows = list(csv.DictReader(open(ROOT / "results.csv", encoding="utf-8")))
B, C = "parallel", "optimized-reduction"


def get(version, threads, sched, chunk):
    for r in rows:
        if (r["Version"], r["Threads"], r["Schedule"], r["Chunk"]) == (version, str(threads), sched, chunk):
            return {"t": float(r["Tiempo promedio (s)"]), "s": float(r["Speedup"]), "e": float(r["Efficiency"])}
    return None


SEQ = get("sequential", 1, "sequential", "-")

# Estilo sobrio tipo LaTeX: Computer Modern, ejes completos, sin rejilla de fondo
plt.rcParams.update({
    "font.family": "serif", "font.serif": ["cmr10"], "mathtext.fontset": "cm",
    "axes.formatter.use_mathtext": True, "axes.unicode_minus": False, "font.size": 10,
    "axes.linewidth": 0.6, "xtick.direction": "in", "ytick.direction": "in",
    "xtick.top": True, "ytick.right": True, "legend.frameon": False, "pdf.fonttype": 42,
})
COL_B, COL_C = "#1f3b73", "#a3271f"


def speedup():
    fig, ax = plt.subplots(figsize=(4.6, 2.9))
    th = [1, 2, 4, 8]
    b = [1.0] + [get(B, p, "static", "default")["s"] for p in th[1:]]
    c = [1.0] + [get(C, p, "dynamic", "64")["s"] for p in th[1:]]
    ax.plot(th, th, "k--", lw=0.8, label="Ideal")
    ax.plot(th, b, "-o", color=COL_B, lw=1.1, ms=4, label="B (static)")
    ax.plot(th, c, "-s", color=COL_C, lw=1.1, ms=4, label="C (dynamic, 64)")
    ax.set_xticks(th)
    ax.set_xlabel("Hilos $p$")
    ax.set_ylabel("Speedup $S(p)$")
    ax.set_xlim(0.6, 8.4)
    ax.set_ylim(0, 8.4)
    ax.legend(loc="upper left")
    fig.savefig(GEN / "speedup.pdf", bbox_inches="tight")


def schedules():
    cfg = [("static", "default"), ("static", "1"), ("static", "8"), ("static", "64"), ("static", "512"),
           ("dynamic", "1"), ("dynamic", "8"), ("dynamic", "64"), ("dynamic", "512"),
           ("guided", "default"), ("guided", "8"), ("guided", "64")]
    labels = [f"{s}\n{'def.' if c == 'default' else c}" for s, c in cfg]
    tb = [get(B, 8, s, c)["t"] for s, c in cfg]
    tc = [get(C, 8, s, c)["t"] for s, c in cfg]
    fig, ax = plt.subplots(figsize=(6.3, 2.7))
    x = range(len(cfg))
    w = 0.38
    ax.bar([i - w / 2 for i in x], tb, w, color="white", edgecolor=COL_B, hatch="////", lw=0.8, label="B (directa)")
    ax.bar([i + w / 2 for i in x], tc, w, color=COL_C, edgecolor=COL_C, lw=0.8, label="C (pares, reduction)")
    ax.axhline(SEQ["t"], color="k", ls="--", lw=0.8)
    ax.text(11.4, SEQ["t"] + 0.006, f"secuencial ({SEQ['t']:.3f} s)", ha="right", fontsize=8)
    # chunk como etiqueta de cada barra y el schedule centrado debajo de su grupo
    ax.set_xticks(list(x))
    ax.set_xticklabels([lb.split("\n")[1] for lb in labels], fontsize=8)
    for name, lo, hi in [("static", 0, 4), ("dynamic", 5, 8), ("guided", 9, 11)]:
        ax.text((lo + hi) / 2, -0.09, name, ha="center", va="top", fontsize=9, style="italic",
                transform=ax.get_xaxis_transform())
    for sep in (4.5, 8.5):
        ax.axvline(sep, color="0.75", lw=0.6)
    ax.set_ylabel("Tiempo (s)")
    ax.set_xlim(-0.6, 11.6)
    ax.set_ylim(0, 0.42)
    ax.legend(loc="upper left", ncol=2, fontsize=8.5)
    fig.savefig(GEN / "schedules.pdf", bbox_inches="tight")


def tabla_resultados():
    cfg = [(1, "secuencial", "--"), (2, "static", "default"), (4, "static", "default"), (8, "static", "default"),
           (8, "static", "8"), (8, "static", "64"), (8, "dynamic", "8"), (8, "dynamic", "64"),
           (8, "guided", "default"), (2, "dynamic", "64"), (4, "dynamic", "64")]
    f = lambda v, d: "--" if v is None else f"{v:.{d}f}"
    out = []
    for p, s, c in cfg:
        b, cc = (SEQ, None) if s == "secuencial" else (get(B, p, s, c), get(C, p, s, c))
        cells = [str(p), f"\\texttt{{{s}}}", c,
                 f(b and b["t"], 3), f(b and b["s"], 2), f(b and b["e"], 2),
                 f(cc and cc["t"], 3), f(cc and cc["s"], 2), f(cc and cc["e"], 2)]
        line = " & ".join(cells) + r" \\"
        if (p, s, c) == (8, "dynamic", "64"):
            line = " & ".join(rf"\textbf{{{v}}}" if i >= 6 else v for i, v in enumerate(cells)) + r" \\"
        out.append(line)
        if (p, s, c) in [(1, "secuencial", "--"), (8, "guided", "default")]:
            out.append(r"\midrule")
    (GEN / "tabla_resultados.tex").write_text("\n".join(out) + "\n", encoding="utf-8")


def tabla_sync():
    data = [("reduction(+:fx[:N], fy[:N])", get(C, 8, "dynamic", "64")),
            ("atomic (4 por par)", get("optimized-atomic", 8, "dynamic", "64")),
            ("critical (1 por par)", get("optimized-critical", 8, "dynamic", "64"))]
    out = [rf"\texttt{{{n}}} & {d['t']:.4f} & {d['s']:.2f} \\" for n, d in data]
    (GEN / "tabla_sync.tex").write_text("\n".join(out) + "\n", encoding="utf-8")


if __name__ == "__main__":
    speedup()
    schedules()
    tabla_resultados()
    tabla_sync()
    print("figuras y tablas en", GEN)
