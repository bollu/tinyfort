#include <stdio.h>
#include <stdlib.h>

void printint64(int i) { printf("%d", i); }
int readint64() {
    int i;
    (void)scanf("%d", &i);
    return i;
}

void assert(int b, char *s) {
    if (!b) {
        printf("assertion failed: %s", s);
        exit(-1);
    }
}
