/*
 * nbody_paralelo.c — Simulación gravitacional N-Body en 2D paralelizada con OpenMP.
 *
 * Laboratorio PCAM + OpenMP — Computación Paralela y Distribuida, UVG
 * Integrantes: Dilary Sarahí Cruz López (231010), Gerardo Fernandez (23763), Jose Ruiz (23719)
 *
 * Compilar:  gcc -O2 -std=c17 -fopenmp nbody_paralelo.c -o nbody_paralelo -lm
 * Ejecutar:  ./nbody_paralelo [N=5000] [pasos=10] [hilos] [static|dynamic|guided] [chunk, 0=default]
 *            [archivo_estado_opcional]
 *   Ej.:     ./nbody_paralelo 5000 10 8 dynamic 64
 *
 * Diseño PCAM:
 *   P  Tarea = calcular la fuerza total sobre el cuerpo i en un paso (iteración del ciclo externo).
 *   C  Todos los hilos solo LEEN x, y, mass de todos los cuerpos. Cada hilo ESCRIBE únicamente
 *      vx[i], vy[i] (y luego x[i], y[i]) de los i que le tocaron  ->  no hay condiciones de carrera.
 *   A  Las N-1 interacciones de un cuerpo se aglomeran en una tarea; con `chunk` se agrupan
 *      varios cuerpos consecutivos por asignación.
 *   M  schedule(runtime): el tipo de schedule y el chunk se eligen por línea de comandos
 *      para comparar static, dynamic y guided sin recompilar.
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include <string.h>

/* Estado de un cuerpo: posición, velocidad y masa. */
typedef struct {
    double x, y;
    double vx, vy;
    double mass;
} Body;

/* Mismo estado inicial que la versión secuencial (misma semilla) para poder comparar resultados. */
void init_system(Body *bodies, int N, int seed) {
    srand(seed);
    for (int i = 0; i < N; i++) {
        bodies[i].x = (double)rand() / RAND_MAX * 100.0;
        bodies[i].y = (double)rand() / RAND_MAX * 100.0;
        bodies[i].vx = 0.0;
        bodies[i].vy = 0.0;
        bodies[i].mass = (double)rand() / RAND_MAX * 10.0 + 1.0;
    }
}

/* Suma de x, y, vx, vy; debe coincidir con la de la versión secuencial. */
double compute_checksum(Body *bodies, int N) {
    double sum = 0.0;
    for (int i = 0; i < N; i++) {
        sum += bodies[i].x + bodies[i].y + bodies[i].vx + bodies[i].vy;
    }
    return sum;
}

/* Guarda el estado final en binario (x, y, vx, vy por cuerpo) para calcular la diferencia máxima. */
void dump_state(const char *path, Body *bodies, int N) {
    FILE *f = fopen(path, "wb");
    if (!f) { perror("dump_state"); return; }
    for (int i = 0; i < N; i++) {
        double v[4] = { bodies[i].x, bodies[i].y, bodies[i].vx, bodies[i].vy };
        fwrite(v, sizeof(double), 4, f);
    }
    fclose(f);
}

int main(int argc, char *argv[]) {
    int N = 5000;
    int steps = 10;
    int seed = 42;
    int threads = omp_get_max_threads();
    char sched_str[20] = "static";
    int chunk = 0;   /* 0 = chunk por defecto de OpenMP */

    if (argc > 1) N = atoi(argv[1]);
    if (argc > 2) steps = atoi(argv[2]);
    if (argc > 3) threads = atoi(argv[3]);
    if (argc > 4) strcpy(sched_str, argv[4]);
    if (argc > 5) chunk = atoi(argv[5]);
    const char *dump_path = (argc > 6) ? argv[6] : NULL;

    omp_set_num_threads(threads);

    /* Mapping: se traduce el texto al tipo de schedule; schedule(runtime) lo usará en los ciclos. */
    omp_sched_t sched_type = omp_sched_static;
    if (strcmp(sched_str, "dynamic") == 0) sched_type = omp_sched_dynamic;
    else if (strcmp(sched_str, "guided") == 0) sched_type = omp_sched_guided;

    omp_set_schedule(sched_type, chunk);

    Body *bodies = (Body *)malloc(N * sizeof(Body));
    init_system(bodies, N, seed);

    double dt = 0.01;       /* paso de tiempo */
    double G = 1.0;         /* constante gravitacional (unidades normalizadas) */
    double epsilon = 1e-9;  /* suavizado: evita dividir entre 0 si dos cuerpos coinciden */

    double start_time = omp_get_wtime();

    /* Los pasos de tiempo NO se paralelizan: el paso t+1 depende del estado final del paso t. */
    for (int step = 0; step < steps; step++) {
        /*
         * Fase 1 (paralela): cada iteración i es una tarea independiente.
         *  - j, dx, dy, fx, fy, ... se declaran dentro del ciclo -> son privadas de cada hilo.
         *  - bodies[*].x, .y, .mass solo se leen; nadie las modifica en esta fase.
         *  - bodies[i].vx/.vy solo los escribe el hilo dueño de i.
         * Por lo tanto no hay race conditions y no se necesita atomic, critical ni reduction.
         */
        #pragma omp parallel for schedule(runtime)
        for (int i = 0; i < N; i++) {
            double fx = 0.0;
            double fy = 0.0;
            for (int j = 0; j < N; j++) {
                if (i != j) {
                    double dx = bodies[j].x - bodies[i].x;
                    double dy = bodies[j].y - bodies[i].y;
                    double dist_sqr = dx*dx + dy*dy + epsilon;
                    double dist = sqrt(dist_sqr);
                    double force = (G * bodies[i].mass * bodies[j].mass) / dist_sqr;

                    fx += force * (dx / dist);
                    fy += force * (dy / dist);
                }
            }
            bodies[i].vx += (fx / bodies[i].mass) * dt;
            bodies[i].vy += (fy / bodies[i].mass) * dt;
        }
        /*
         * Barrera implícita al final del parallel for: ningún hilo mueve cuerpos hasta que TODAS
         * las fuerzas se calcularon con las posiciones del instante t.
         */

        /* Fase 2 (paralela): cada hilo actualiza solo la posición de sus propios cuerpos. */
        #pragma omp parallel for schedule(runtime)
        for (int i = 0; i < N; i++) {
            bodies[i].x += bodies[i].vx * dt;
            bodies[i].y += bodies[i].vy * dt;
        }
        /* Segunda barrera implícita: el siguiente paso arranca con todas las posiciones actualizadas. */
    }

    double end_time = omp_get_wtime();

    /* Número real de hilos del equipo (puede diferir del solicitado). */
    int actual_threads = 1;
    #pragma omp parallel
    {
        #pragma omp single
        actual_threads = omp_get_num_threads();
    }

    printf("Student: Jose Ruiz, Dilary Cruz, Gerardo Fernandez\n");
    printf("Executable: %s\n", argv[0]);
    printf("Mode: parallel\n");
    printf("N: %d | Steps: %d | Seed: %d\n", N, steps, seed);
    printf("Threads requested: %d | Threads used: %d\n", threads, actual_threads);
    if (chunk == 0) {
        printf("Schedule: %s | Chunk: default\n", sched_str);
    } else {
        printf("Schedule: %s | Chunk: %d\n", sched_str, chunk);
    }
    printf("Elapsed: %.3f s\n", end_time - start_time);
    printf("Checksum: %lf\n", compute_checksum(bodies, N));
    printf("Processors reported by OpenMP: %d | Max threads (omp_get_max_threads): %d\n", omp_get_num_procs(), omp_get_max_threads());

    if (dump_path) dump_state(dump_path, bodies, N);

    free(bodies);
    return 0;
}
