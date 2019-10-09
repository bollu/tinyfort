import fputc(c: char, f: FILE): void
import fputs(s: char[100], f: FILE): void
import sqrtl(i: int): int
import readint64(): int
import stdout: FILE


def main() : int {
      ntriples : int = 0;
      NMAX : int = 100000000;

      for x : int = 0; x <= NMAX; x = x + 1  {
              for y : int = x; y <= NMAX; y = y + 1  {
                      for z : int = y; z <= NMAX; z = z + 1 {
                              if x*x + y*y == z*z  {
                              print(x);
                              fputc(' ',stdout);
                              print(y);
                              fputc(' ',stdout);
                              print(z);
                              fputc('\n', stdout);
                              ntriples = ntriples + 1;
                              }
                      }
              }
      }
      # fputs("num triples: ", stdout);
      print(ntriples);
      return 0;
}
