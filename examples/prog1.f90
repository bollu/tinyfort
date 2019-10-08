import fputc(c: char, f: FILE): void
import fputs(s: char[100], f: FILE): void
import readint64(): int
import stdout: FILE

def pow(base: int, p: int): int {
      prod : int = 1;
      for i: int = 1; i <= p; i = i+ 1 {
        prod = prod * base;
      }
      return prod;
}

def prog(n: int, k: int): int {
      sum : int = 0;
      for i: int = 0; i <= n; i = i + 1 {
        sum = sum + pow(i, k);
      }
      return sum;
}

def main() : int {
      n : int;
      k : int;

      n = readint64();
      k = readint64();
      print(pow(n, k));
      return 0;
}
