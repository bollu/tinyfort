#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void printint64(int i) { printf("%d", i); }
void printstring(char *s) { printf("%s", s); }
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

int argvlen(int i, char **argv) {
    return strlen(argv[i]);
}

void getargv(int i, char **argv, char *out) {
    strcpy(out, argv[i]);
}
