#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include <string.h>

/* Estrategia de sincronizacion para la version optimizada (se elige al compilar):
 *   (default)          reduction(+:fx[:N], fy[:N])  -> acumuladores privados por hilo
 *   -DSYNC_ATOMIC      #pragma omp atomic en cada acumulacion
 *   -DSYNC_CRITICAL    #pragma omp critical en cada par (i,j)
 */
#if defined(SYNC_ATOMIC)
#define SYNC_NAME "atomic"
#define REDUCTION_CLAUSE
#elif defined(SYNC_CRITICAL)
#define SYNC_NAME "critical"
#define REDUCTION_CLAUSE
#else
#define SYNC_NAME "reduction"
#define REDUCTION_CLAUSE reduction(+:fx[:N], fy[:N])
#endif

typedef struct {
    double x, y;
    double vx, vy;
    double mass;
} Body;

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

double compute_checksum(Body *bodies, int N) {
    double sum = 0.0;
    for (int i = 0; i < N; i++) {
        sum += bodies[i].x + bodies[i].y + bodies[i].vx + bodies[i].vy;
    }
    return sum;
}

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
    int chunk = 0;

    if (argc > 1) N = atoi(argv[1]);
    if (argc > 2) steps = atoi(argv[2]);
    if (argc > 3) threads = atoi(argv[3]);
    if (argc > 4) strcpy(sched_str, argv[4]);
    if (argc > 5) chunk = atoi(argv[5]);
    const char *dump_path = (argc > 6) ? argv[6] : NULL;

    omp_set_num_threads(threads);

    omp_sched_t sched_type = omp_sched_static;
    if (strcmp(sched_str, "dynamic") == 0) sched_type = omp_sched_dynamic;
    else if (strcmp(sched_str, "guided") == 0) sched_type = omp_sched_guided;
    
    omp_set_schedule(sched_type, chunk);

    Body *bodies = (Body *)malloc(N * sizeof(Body));
    init_system(bodies, N, seed);

    double dt = 0.01;
    double G = 1.0;
    double epsilon = 1e-9;

    double *fx = (double *)calloc(N, sizeof(double));
    double *fy = (double *)calloc(N, sizeof(double));

    double start_time = omp_get_wtime();

    for (int step = 0; step < steps; step++) {
        memset(fx, 0, N * sizeof(double));
        memset(fy, 0, N * sizeof(double));

        #pragma omp parallel for schedule(runtime) REDUCTION_CLAUSE
        for (int i = 0; i < N; i++) {
            for (int j = i + 1; j < N; j++) {
                double dx = bodies[j].x - bodies[i].x;
                double dy = bodies[j].y - bodies[i].y;
                double dist_sqr = dx*dx + dy*dy + epsilon;
                double dist = sqrt(dist_sqr);
                double force = (G * bodies[i].mass * bodies[j].mass) / dist_sqr;
                
                double f_x = force * (dx / dist);
                double f_y = force * (dy / dist);

#if defined(SYNC_ATOMIC)
                #pragma omp atomic
                fx[i] += f_x;
                #pragma omp atomic
                fy[i] += f_y;
                #pragma omp atomic
                fx[j] -= f_x;
                #pragma omp atomic
                fy[j] -= f_y;
#elif defined(SYNC_CRITICAL)
                #pragma omp critical(force_update)
                {
                    fx[i] += f_x;
                    fy[i] += f_y;
                    fx[j] -= f_x;
                    fy[j] -= f_y;
                }
#else
                fx[i] += f_x;
                fy[i] += f_y;
                fx[j] -= f_x;
                fy[j] -= f_y;
#endif
            }
        }

        #pragma omp parallel for schedule(runtime)
        for (int i = 0; i < N; i++) {
            bodies[i].vx += (fx[i] / bodies[i].mass) * dt;
            bodies[i].vy += (fy[i] / bodies[i].mass) * dt;
            bodies[i].x += bodies[i].vx * dt;
            bodies[i].y += bodies[i].vy * dt;
        }
    }

    double end_time = omp_get_wtime();
    int actual_threads = 1;
    #pragma omp parallel
    {
        #pragma omp single
        actual_threads = omp_get_num_threads();
    }

    printf("Student: Jose Ruiz, Dilary Cruz, Gerardo Fernandez\n");
    printf("Executable: %s\n", argv[0]);
    printf("Mode: parallel-optimized (sync: %s)\n", SYNC_NAME);
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
    free(fx);
    free(fy);
    return 0;
}
