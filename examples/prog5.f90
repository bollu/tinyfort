import fputc(c: char, f: FILE): void
import fputs(s: char[100], f: FILE): void
import sqrtl(i: int): int
import readint64(): int
import stdout: FILE


def main() : int {
    n: int = readint64();
    for i:int = 0; i < (1 << n); i = i + 1 {
      fputs("{ ", stdout);
      for j:int = 0; j < n; j = j + 1 {
        if i & (1 << j) != 0 {
            print(j+1);
            fputc(' ', stdout);
          }
      }
      fputc('}', stdout);
      fputc('\n', stdout);
    }
    
    return 0;
}
