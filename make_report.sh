#!/bin/bash
# Genera report.pdf desde report.md (pandoc + xelatex). Si faltan capturas .png
# las reemplaza por un aviso, para poder generar un borrador.
cd "$(dirname "$0")"
TMP=$(mktemp -t report).md
cp report.md "$TMP"
for img in compile sequential parallel optimized hardware; do
  if [ ! -f "evidence/$img.png" ]; then
    echo "AVISO: falta evidence/$img.png"
    sed -i.bak "s#^!\[\(.*\)\](evidence/$img.png).*#**[Captura pendiente: $img.png]**#" "$TMP"
  fi
done
pandoc "$TMP" -o report.pdf --pdf-engine=xelatex --resource-path=. -V colorlinks=true
rm -f "$TMP" "$TMP.bak"
echo "report.pdf generado ($(pdfinfo report.pdf 2>/dev/null | awk '/Pages/{print $2}') paginas)"
