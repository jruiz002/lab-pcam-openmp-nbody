#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

int main() {
    int N = 100;
    double *arr = malloc(N * sizeof(double));
    for (int i = 0; i < N; i++) arr[i] = 0.0;

    #pragma omp parallel for reduction(+:arr[:N])
    for (int i = 0; i < N; i++) {
        for (int j = i + 1; j < N; j++) {
            arr[i] += 1.0;
            arr[j] += 1.0;
        }
    }

    double sum = 0;
    for (int i = 0; i < N; i++) sum += arr[i];
    printf("Sum: %f\n", sum);
    free(arr);
    return 0;
}
