import fputc(c: char, f: FILE): void
import fputs(s: char[100], f: FILE): void
import sqrtl(i: int): int
import readint64(): int
import stdout: FILE


def main() : int {
      ntriples : int = 0;
      # NMAX : int = 100000000;
      NMAX : int = 3;

      for z : int = 0; z <= NMAX; z = z + 1  {
              for x : int = 0; x < NMAX; x = x + 1  {
                      for y : int = 0; y < NMAX; y = y + 1 {
                              if (x*x + y*y) == z*z  {
                                      print(x);
                                      fputc(' ',stdout);
                                      print(y);
                                      fputc(' ',stdout);
                                      print(z);
                                      fputc('\n', stdout);
                              }
                      }
              }
      }
      return 0;
}
