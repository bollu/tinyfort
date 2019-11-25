import fputc(c: char, f: FILE): void
import fputs(s: char[100], f: FILE): void
import readint64(): int
import stdout: FILE


def main() : int {
      n : int;
      n = readint64();

      # whether the number is prime
      isprime : bool[n+1];
      isprime[2] = true;

      for i : int = 3; i <= n; i = i + 1  {
        isprime[i] = true;

        for div : int = 2; div <= i - 1; div = div + 1 {

          if isprime[div] && i % div == 0 {
             isprime[i] = false;
           }
        }
      }

      sum : int = 0;
      for i: int = 2; i <= n; i = i + 1  {
          if (isprime[i]) { sum = sum  + i;}
      }
      print(sum);

      return 0;
}
