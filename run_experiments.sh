#!/bin/bash
# Compila las versiones, ejecuta cada configuracion REPS veces y genera results.csv
# con tiempo promedio, Speedup, Efficiency, checksum y diferencia maxima vs secuencial.
# Uso: bash run_experiments.sh   (CC=gcc bash run_experiments.sh en Linux)
set -e
cd "$(dirname "$0")"

N=5000; STEPS=10; REPS=3
CC=${CC:-$(command -v gcc-14 || command -v gcc-13 || command -v gcc)}
CFLAGS="-O2 -std=c17 -fopenmp"
BIN=build; OUT=results.csv
mkdir -p $BIN evidence

echo "Compilador: $CC"
$CC $CFLAGS src/nbody_seq.c       -o $BIN/nbody_seq       -lm
$CC $CFLAGS src/nbody_parallel.c  -o $BIN/nbody_parallel  -lm
$CC $CFLAGS src/nbody_optimized.c -o $BIN/nbody_optimized -lm
$CC $CFLAGS -DSYNC_ATOMIC   src/nbody_optimized.c -o $BIN/nbody_optimized_atomic   -lm
$CC $CFLAGS -DSYNC_CRITICAL src/nbody_optimized.c -o $BIN/nbody_optimized_critical -lm
$CC -O2 src/compare.c -o $BIN/compare -lm

echo "Version,Threads,Schedule,Chunk,Tiempo promedio (s),Speedup,Efficiency,Checksum,MaxDiff vs seq" > $OUT

T1=""
# run_test <version> <exec> <threads> <schedule> <chunk|-> [N]
run_test() {
    local version=$1 exec=$2 threads=$3 sched=$4 chunk=$5
    local args="$N $STEPS" total=0 elapsed out checksum maxdiff
    if [ "$version" = "sequential" ]; then
        args="$args $BIN/dump_test.bin"
    else
        local c=0; [ "$chunk" != "default" ] && c=$chunk
        args="$args $threads $sched $c $BIN/dump_test.bin"
    fi
    for i in $(seq $REPS); do
        out=$($exec $args)
        elapsed=$(echo "$out" | awk '/^Elapsed:/ {print $2}')
        total=$(echo "$total + $elapsed" | bc -l)
    done
    checksum=$(echo "$out" | awk '/^Checksum:/ {print $2}')
    local avg; avg=$(echo "scale=6; $total / $REPS" | bc -l)
    if [ "$version" = "sequential" ]; then
        cp $BIN/dump_test.bin $BIN/dump_seq.bin
        T1=$avg; maxdiff="0.000e+00"
    else
        maxdiff=$($BIN/compare $BIN/dump_seq.bin $BIN/dump_test.bin)
    fi
    local speedup eff
    speedup=$(echo "scale=4; $T1 / $avg" | bc -l)
    eff=$(echo "scale=4; $speedup / $threads" | bc -l)
    printf "%s,%s,%s,%s,%.4f,%.3f,%.3f,%s,%s\n" "$version" "$threads" "$sched" "$chunk" "$avg" "$speedup" "$eff" "$checksum" "$maxdiff" >> $OUT
    printf "%-22s T=%-2s %-10s chunk=%-7s avg=%.4f s  S=%.3f E=%.3f  maxdiff=%s\n" "$version" "$threads" "$sched" "$chunk" "$avg" "$speedup" "$eff" "$maxdiff"
}

echo "== A: secuencial =="
run_test sequential $BIN/nbody_seq 1 sequential -

# Configuraciones obligatorias del enunciado (+ chunks extra para Agglomeration)
CONFIGS="2:static:default 4:static:default 8:static:default 8:static:8 8:static:64
8:dynamic:8 8:dynamic:64 8:guided:default
8:static:1 8:static:512 8:dynamic:1 8:dynamic:512 8:guided:8 8:guided:64"

echo "== B: paralela directa =="
for cfg in $CONFIGS; do IFS=: read t s c <<< "$cfg"; run_test parallel $BIN/nbody_parallel $t $s $c; done

echo "== C: optimizada (reduction) =="
for cfg in $CONFIGS; do IFS=: read t s c <<< "$cfg"; run_test optimized-reduction $BIN/nbody_optimized $t $s $c; done
# Escalado de la optimizada con la mejor config
run_test optimized-reduction $BIN/nbody_optimized 2 dynamic 64
run_test optimized-reduction $BIN/nbody_optimized 4 dynamic 64

echo "== C: optimizada, alternativas de sincronizacion (8 dynamic 64) =="
run_test optimized-atomic   $BIN/nbody_optimized_atomic   8 dynamic 64
run_test optimized-critical $BIN/nbody_optimized_critical 8 dynamic 64

rm -f $BIN/dump_test.bin
echo "Listo. Ver $OUT"
