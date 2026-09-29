# N-Body 2D con PCAM + OpenMP (C)

**Equipo:** Jose Ruiz · Dilary Cruz · Gerardo Fernandez

Simulación gravitacional N-Body en 2D en tres versiones:

| Versión | Archivo | Descripción |
|---|---|---|
| A | `src/nbody_seq.c` | Secuencial, doble ciclo completo i × j (baseline) |
| B | `src/nbody_parallel.c` | Paralela directa: cada cuerpo `i` acumula su fuerza |
| C | `src/nbody_optimized.c` | Optimizada: cada par (i,j) una sola vez; carrera resuelta con `reduction` (o `-DSYNC_ATOMIC` / `-DSYNC_CRITICAL` para compararlas) |

`src/compare.c` calcula la diferencia máxima entre dos estados finales (volcados binarios).

## Requisitos
GCC con OpenMP. En macOS: `brew install gcc` (se usa `gcc-14`; el `gcc` del sistema es clang y no soporta `-fopenmp`). En Linux basta `gcc`. Para el PDF: `pandoc` y `xelatex`.

## Compilar
```bash
mkdir -p build
gcc-14 -O2 -std=c17 -fopenmp src/nbody_seq.c       -o build/nbody_seq       -lm
gcc-14 -O2 -std=c17 -fopenmp src/nbody_parallel.c  -o build/nbody_parallel  -lm
gcc-14 -O2 -std=c17 -fopenmp src/nbody_optimized.c -o build/nbody_optimized -lm
```

## Ejecutar
```bash
./build/nbody_seq 5000 10                      # N pasos [dump.bin]
./build/nbody_parallel 5000 10 8 dynamic 64    # N pasos threads schedule chunk [dump.bin]
./build/nbody_optimized 5000 10 8 dynamic 64   # schedule: static | dynamic | guided ; chunk 0 = default
```
Seed fija = 42. El tiempo se mide con `omp_get_wtime()`. Cada ejecución imprime alumno(s), ejecutable, N/pasos/seed, hilos pedidos y usados, schedule, chunk, tiempo, checksum y `omp_get_num_procs()` / `omp_get_max_threads()`.

## Experimentos completos
```bash
bash run_experiments.sh          # CC=gcc bash run_experiments.sh en Linux
```
Compila todo en `build/`, ejecuta cada configuración 3 veces (N=5000, 10 pasos) y escribe `results.csv` con tiempo promedio, Speedup, Efficiency, checksum y diferencia máxima vs la secuencial (tarda ~30 s).

## Reporte
`report.md` → `report.pdf` con `bash make_report.sh`. Las capturas de pantalla van en `evidence/` (ver `evidence/README.md`).

## Estructura
```
src/  build/(ignorado)  evidence/  results.csv  run_experiments.sh  make_report.sh  report.md  report.pdf  README.md
```
