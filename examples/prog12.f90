import fputc(c: char, f: FILE): void
import ungetc(c: char, f: FILE): void
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

# Consume till an alphabet. Return whether we encountered EOF while consume
# whitespace
def consume_whitespace(f: FILE): bool {
  while true {
    c: char = fgetc(f);
    if  int(c) == -1  { return true; }
    
    if  c != ' ' && c != '\n' && c == '\t' { ungetc(c, f); return false; }
  }
  # unreachable
  return false;
}

# consume till not an alphabet. Write word into word. store into bool
# whether we reached EOF when consuming word
def consume_word(f: FILE, word: char[100]): bool {
  i: int = 0;

  while(true) {
    c: char = fgetc(f);
    if int(c) == -1  {
      word[i] = '\0';
      return true;
    }

    if c == ' ' || c == '\n' || c == '\t' { word[i] = '\0'; ungetc(c, f); return false; }

    word[i] = c;
    i = i + 1;
  }

  # unreachable
  return true;
}


def streq(c: char[0], d: char[0]): bool{
   i: int = 0;

   while(true) {
     if c[i] == '\0' {
        return d[i] == '\0';
     } else {
       if c[i] != d[i] { 
         return false;
       }
       i = i + 1;
     }
   }
  # unreachable
  return false;
}

def main(argc: int, argv: char[0]) : int {
    if argc != 2 {
        fputs("usage: prog12 <path-to-file>\n", stdout);
        return 0;
    }


    # get argv
    path: char[argvlen(1, argv)+10];
    getargv(1, argv, path);

    fputs("PATH: |", stdout);
    fputs(path, stdout);
    fputs("|", stdout);
    fflush(stdout);

    f: FILE = fopen(path, "r");

    MAXWORDS : int= 10000;
    MAXWORDLEN : int = 500;

    # stores words
    words: char[MAXWORDLEN, MAXWORDS];
    ## stores counts of words
    wordcount: int[MAXWORDS];

    ## the array is small enough that linear search is good enough.
    ## no need for fancy hashing
    for i: int = 0; i < MAXWORDS; i = i + 1 { wordcount[i] = 0; }

    have_data: bool = true;
    while(have_data) {
        have_data = consume_whitespace(f);

        if (have_data) {
            # stores current word
            curword: char[MAXWORDLEN];
            have_data = consume_word(f, curword);
        }
    }

    for i: int = 0; i < 26; i = i + 1 {
        print(words[i]);
        fputc(':', stdout);
        print(wordcount[i]);
        fputc('\n', stdout);
    }
    return 0;
}
