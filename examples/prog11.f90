import fputc(c: char, f: FILE): void
import fputs(s: char[0], f: FILE): void
import sqrtl(i: int): int
import readint64(): int
import assert(b: bool, s:char[0]): int
import argvlen(ix: int, argv:char[0]): int
import getargv(ix: int, argv: char[0], out: char[0]): void
import fopen(path: char[0], mode: char[0]): FILE
import fread(c: char[0], size: int, nmemb: int, f: FILE): int
import fflush(f: FILE): void
import stdout: FILE


def main(argc: int, argv: char[0]) : int {
    if argc != 2 { 
        fputs("usage: prog10 <path-to-file>\n", stdout);
        return 0;
    }


    path: char[argvlen(1, argv)+10];
    getargv(1, argv, path);

    fputs("PATH: |", stdout);
    fputs(path, stdout);
    fputs("|", stdout);
    fflush(stdout);

    f: FILE = fopen(path, "r");

    # TODO: use fseek to read this correctly
    data: char[9999];

    for i : int = 0; i < 9999; i = i + 1 {
        data[i] = '\0';
    }
    

    fputs("reading...", stdout); fflush(stdout);
    nread: int = fread(data, 9999, 1, f);

    print(nread);
    fflush(stdout);

    # TODO: need syntax for casting. create histogram table, that's about it.

    return 0;
}
