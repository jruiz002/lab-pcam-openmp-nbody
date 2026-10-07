"""Compila los PDF del laboratorio con LaTeX.

Uso:  python report_build/build.py
Salida: entrega/explicacion_pcam.pdf, entrega/evidencia.pdf y report.pdf
Requiere: matplotlib y una distribución LaTeX con latexmk (p. ej. TinyTeX).
"""
import pathlib
import shutil
import subprocess
import sys

HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE.parent
DOCS = {"explicacion_pcam": ROOT / "entrega", "evidencia": ROOT / "entrega", "report": ROOT}

subprocess.run([sys.executable, HERE / "figuras.py"], check=True)
for doc, dest in DOCS.items():
    subprocess.run(["latexmk", "-pdf", "-interaction=nonstopmode", "-halt-on-error",
                    "-outdir=out", f"{doc}.tex"], cwd=HERE, check=True, stdout=subprocess.DEVNULL)
    dest.mkdir(exist_ok=True)
    shutil.copy(HERE / "out" / f"{doc}.pdf", dest / f"{doc}.pdf")
    print("generado", dest / f"{doc}.pdf")
