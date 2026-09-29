# Evidencia de ejecución

Los `.txt` son la salida real de los programas (generados con los comandos del README).
Los `.png` requeridos por el enunciado deben ser **capturas de pantalla de la terminal**
ejecutando esos mismos comandos, con estos nombres:

| Archivo | Comando a capturar |
|---|---|
| compile.png | `gcc-14 -O2 -std=c17 -fopenmp src/nbody_seq.c -o build/nbody_seq -lm` (y las otras dos) |
| sequential.png | `./build/nbody_seq 5000 10` |
| parallel.png | `./build/nbody_parallel 5000 10 8 dynamic 64` |
| optimized.png | `./build/nbody_optimized 5000 10 8 dynamic 64` |
| hardware.png | cualquiera de las salidas (línea `Processors reported by OpenMP ... Max threads`) |

Después de guardarlas: `bash make_report.sh` regenera `report.pdf` con las imágenes incluidas.
