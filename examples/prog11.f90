import fputc(c: char, f: FILE): void
import fgetc(f: FILE): char 
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
        fputs("usage: prog11 <path-to-file>\n", stdout);
        return 0;
    }


    path: char[argvlen(1, argv)+10];
    getargv(1, argv, path);

    fputs("PATH: |", stdout);
    fputs(path, stdout);
    fputs("|", stdout);
    fflush(stdout);

    f: FILE = fopen(path, "r");

    histogram: int[26];
    for i: int = 0; i < 26; i = i + 1 { histogram[i] = 0; }

    have_data : bool = true;
    
    while(have_data) {
        c: char = fgetc(f);
        # EOF is -1
        have_data = int(c) != -1;

        if c >= 'a' && c <= 'z' {
           histogram[c - 'a'] = histogram[c - 'a'] + 1;
        }
        elif c >= 'A' && c <= 'Z' {
           histogram[c - 'A'] = histogram[c - 'A'] + 1;
        }
    }

    for i: int = 0; i < 26; i = i + 1 {
        fputc(char(i + int('a')), stdout);
        fputc(':', stdout);
        print(histogram[i]);
        fputc('\n', stdout);
    }
    return 0;
}
