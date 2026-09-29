#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* Uso: ./compare ref.bin test.bin
 * Imprime la diferencia absoluta maxima entre x, y, vx, vy de dos volcados. */
int main(int argc, char *argv[]) {
    if (argc < 3) { fprintf(stderr, "uso: %s ref.bin test.bin\n", argv[0]); return 1; }
    FILE *a = fopen(argv[1], "rb"), *b = fopen(argv[2], "rb");
    if (!a || !b) { perror("fopen"); return 1; }
    double va[4], vb[4], maxdiff = 0.0;
    while (fread(va, sizeof(double), 4, a) == 4 && fread(vb, sizeof(double), 4, b) == 4)
        for (int k = 0; k < 4; k++) {
            double d = fabs(va[k] - vb[k]);
            if (d > maxdiff) maxdiff = d;
        }
    printf("%.3e\n", maxdiff);
    fclose(a); fclose(b);
    return 0;
}
