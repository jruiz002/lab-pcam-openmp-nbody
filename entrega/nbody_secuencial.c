/*
 * nbody_secuencial.c — Simulación gravitacional N-Body en 2D (versión secuencial, baseline).
 *
 * Laboratorio PCAM + OpenMP — Computación Paralela y Distribuida, UVG
 * Integrantes: Dilary Sarahí Cruz López (231010), Gerardo Fernandez (23763), Jose Ruiz (23719)
 *
 * Compilar:  gcc -O2 -std=c17 -fopenmp nbody_secuencial.c -o nbody_secuencial -lm
 * Ejecutar:  ./nbody_secuencial [N=5000] [pasos=10] [archivo_estado_opcional]
 *
 * No usa paralelismo: OpenMP solo se incluye para medir el tiempo con omp_get_wtime()
 * y reportar omp_get_num_procs(), igual que las versiones paralelas.
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

/* Posiciones aleatorias en [0,100)^2, en reposo, masa en [1,11). Semilla fija para reproducibilidad. */
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

/* Suma de x, y, vx, vy de todos los cuerpos; sirve para comparar resultados entre versiones. */
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

    if (argc > 1) N = atoi(argv[1]);
    if (argc > 2) steps = atoi(argv[2]);

    const char *dump_path = (argc > 3) ? argv[3] : NULL;

    Body *bodies = (Body *)malloc(N * sizeof(Body));
    init_system(bodies, N, seed);

    double dt = 0.01;       /* paso de tiempo */
    double G = 1.0;         /* constante gravitacional (unidades normalizadas) */
    double epsilon = 1e-9;  /* suavizado: evita dividir entre 0 si dos cuerpos coinciden */

    double start_time = omp_get_wtime();

    for (int step = 0; step < steps; step++) {
        /* Fase 1: fuerza total sobre cada cuerpo i usando las posiciones del instante t. O(N^2). */
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
            /* Solo se actualiza la velocidad: las posiciones deben seguir siendo las de t. */
            bodies[i].vx += (fx / bodies[i].mass) * dt;
            bodies[i].vy += (fy / bodies[i].mass) * dt;
        }

        /* Fase 2: con todas las velocidades nuevas, se mueven los cuerpos (posición en t+1). */
        for (int i = 0; i < N; i++) {
            bodies[i].x += bodies[i].vx * dt;
            bodies[i].y += bodies[i].vy * dt;
        }
    }

    double end_time = omp_get_wtime();

    printf("Student: Jose Ruiz, Dilary Cruz, Gerardo Fernandez\n");
    printf("Executable: %s\n", argv[0]);
    printf("Mode: sequential\n");
    printf("N: %d | Steps: %d | Seed: %d\n", N, steps, seed);
    printf("Threads requested: 1 | Threads used: 1\n");
    printf("Schedule: sequential | Chunk: -\n");
    printf("Elapsed: %.3f s\n", end_time - start_time);
    printf("Checksum: %lf\n", compute_checksum(bodies, N));
    printf("Processors reported by OpenMP: %d | Max threads (omp_get_max_threads): %d\n", omp_get_num_procs(), omp_get_max_threads());

    if (dump_path) dump_state(dump_path, bodies, N);

    free(bodies);
    return 0;
}
