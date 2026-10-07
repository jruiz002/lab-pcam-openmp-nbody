# N-Body 2D con PCAM + OpenMP (C)

**Equipo:**
* Jose Ruiz — 23719
* Dilary Cruz — 231010
* Gerardo Fernandez — 23763

Simulación gravitacional N-Body en 2D implementada en **C (C17)** con **OpenMP**, estructurada bajo la metodología de diseño paralelo **PCAM** (Partition, Communication, Agglomeration, Mapping).

---

## 1. Versiones implementadas

| Versión | Archivo | Descripción | Sincronización / Estrategia |
|---|---|---|---|
| **A (Secuencial)** | `src/nbody_seq.c` | Baseline secuencial. Doble ciclo completo $N \times N$. | Ninguna |
| **B (Paralela directa)** | `src/nbody_parallel.c` | Paralelismo a nivel de cuerpo. Cada hilo calcula la fuerza sobre sus cuerpos $i$. | Sin carrera (acumuladores privados por cuerpo) |
| **C (Optimizada)** | `src/nbody_optimized.c` | Simetría de Newton ($F_{ij} = -F_{ji}$). Calcula cada par $(i,j)$ una sola vez ($N(N-1)/2$). | `reduction(+:fx[:N], fy[:N])` (por defecto) |

> **Nota:** La versión C también soporta compilación condicional con `-DSYNC_ATOMIC` y `-DSYNC_CRITICAL` para comparar experimentalmente el costo de contención frente a la reducción de arreglos en OpenMP.
> `src/compare.c` compara dos volcados binarios de estado final y reporta la diferencia absoluta máxima para verificar la corrección numérica.

---

## 2. Requisitos e Instalación

### Compilador C con soporte OpenMP
* **macOS:** Instalar GCC vía Homebrew:
  ```bash
  brew install gcc
  ```
  *(Se utiliza `gcc-14` o `gcc-13` ya que el `gcc`/`clang` por defecto de Xcode no soporta `-fopenmp`).*
* **Linux (Ubuntu/Debian):**
  ```bash
  sudo apt update && sudo apt install build-essential gcc
  ```
* **Windows:** MinGW-w64 (con soporte OpenMP/pthreads) o entorno WSL.

### Generación de Reporte en PDF (Opcional)
* Requiere `pandoc` y motor LaTeX con XeLaTeX:
  * **macOS:** `brew install pandoc` y MacTeX / BasicTeX / TinyTeX.
  * **Linux:** `sudo apt install pandoc texlive-xetex texlive-fonts-recommended`

---

## 3. Compilación

Crea el directorio `build/` y compila los ejecutables:

```bash
mkdir -p build

# En macOS (usando gcc de Homebrew):
gcc-14 -O2 -std=c17 -fopenmp src/nbody_seq.c       -o build/nbody_seq       -lm
gcc-14 -O2 -std=c17 -fopenmp src/nbody_parallel.c  -o build/nbody_parallel  -lm
gcc-14 -O2 -std=c17 -fopenmp src/nbody_optimized.c -o build/nbody_optimized -lm

# O en Linux / entornos con gcc estándar:
gcc -O2 -std=c17 -fopenmp src/nbody_seq.c       -o build/nbody_seq       -lm
gcc -O2 -std=c17 -fopenmp src/nbody_parallel.c  -o build/nbody_parallel  -lm
gcc -O2 -std=c17 -fopenmp src/nbody_optimized.c -o build/nbody_optimized -lm
```

Para compilar las variantes de sincronización de la versión C y la utilidad de comparación:
```bash
# Variante con omp atomic:
gcc-14 -O2 -std=c17 -fopenmp -DSYNC_ATOMIC src/nbody_optimized.c -o build/nbody_optimized_atomic -lm

# Variante con omp critical:
gcc-14 -O2 -std=c17 -fopenmp -DSYNC_CRITICAL src/nbody_optimized.c -o build/nbody_optimized_critical -lm

# Herramienta de comparación binaria:
gcc-14 -O2 src/compare.c -o build/compare -lm
```

---

## 4. Ejecución

### Versión A — Secuencial
```bash
./build/nbody_seq <N> <pasos> [dump.bin]
```
**Ejemplo:**
```bash
./build/nbody_seq 5000 10
```

### Versión B — Paralela directa
```bash
./build/nbody_parallel <N> <pasos> <threads> <schedule> <chunk> [dump.bin]
```
* `schedule`: `static` | `dynamic` | `guided`
* `chunk`: tamaño del bloque (`0` = default de OpenMP)
* `dump.bin` (opcional): ruta de archivo binario para volcar el estado final $(x, y, v_x, v_y)$.

**Ejemplo:**
```bash
./build/nbody_parallel 5000 10 8 dynamic 64
```

### Versión C — Optimizada (pares únicos)
```bash
./build/nbody_optimized <N> <pasos> <threads> <schedule> <chunk> [dump.bin]
```
**Ejemplo:**
```bash
./build/nbody_optimized 5000 10 8 dynamic 64
```

> **Salida estándar:** Cada programa reporta automáticamente los datos de los integrantes, modo de ejecución, parámetros ($N$, pasos, seed fija = 42), hilos solicitados y usados, schedule, chunk, tiempo transcurrido (`omp_get_wtime()`), checksum de control y procesadores detectados por OpenMP (`omp_get_num_procs()` / `omp_get_max_threads()`).

---

## 5. Experimentos Automatizados

Para ejecutar la batería completa de pruebas (3 repeticiones por configuración, cálculo de Speedup, Eficiencia, Checksum y validación numérica con `compare`):

```bash
# En macOS:
bash run_experiments.sh

# En Linux:
CC=gcc bash run_experiments.sh
```

El script genera el archivo `results.csv` con todos los resultados consolidados.

---

## 6. Generación del Reporte

El reporte técnico con el diseño PCAM, análisis de race conditions, respuestas a las preguntas de análisis, conclusiones y evidencias de ejecución se encuentra en `report.md`.

Para compilar el documento en `report.pdf`:
```bash
bash make_report.sh
```

Las capturas de pantalla de la terminal se encuentran en `evidence/` (`compile.png`, `sequential.png`, `parallel.png`, `optimized.png`, `hardware.png`).

---

## 7. Estructura del Proyecto

```
lab-pcam-nbody/
├── src/
│   ├── nbody_seq.c           # Versión A (Secuencial baseline)
│   ├── nbody_parallel.c      # Versión B (Paralela directa por cuerpo)
│   ├── nbody_optimized.c     # Versión C (Optimizada por pares con reduction)
│   └── compare.c             # Verificación de diferencias numéricas máximas
├── evidence/
│   ├── README.md             # Especificación de capturas requeridas
│   ├── compile.png           # Captura de compilación limpia
│   ├── sequential.png        # Captura ejecución secuencial
│   ├── parallel.png          # Captura ejecución paralela directa
│   ├── optimized.png         # Captura ejecución optimizada
│   └── hardware.png          # Captura de procesadores / hilos OpenMP
├── results.csv               # Resultados consolidados de experimentos
├── report.md                 # Documento fuente del reporte técnico
├── report.pdf                # Reporte final compilado en formato PDF
├── run_experiments.sh        # Script automatizado de experimentación
├── make_report.sh            # Script de compilación del reporte PDF
└── README.md                 # Documentación e instrucciones de uso
```
