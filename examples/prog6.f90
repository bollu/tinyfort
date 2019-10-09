import fputc(c: char, f: FILE): void
import fputs(s: char[100], f: FILE): void
import sqrtl(i: int): int
import readint64(): int
import stdout: FILE

def insertionsort(n: int, xs: int[n]): void {
  # i marks the current unsorted element
  for i:int = 1; i < n; i = i + 1 {
     for j:int = i; j > 0; j = j - 1 {
        if xs[j - 1] > xs[j] {
           temp: int = xs[j];
           xs[j] = xs[j - 1];
           xs[j - 1] = temp;
        }
     }
    
  }
}


def main() : int {
    n: int = 0;
    n = readint64();
    a: int[n];

    for i: int = 0; i < n; i = i + 1 {
       a[i] = readint64();
    }

    insertionsort(n, a);

    for i: int = 0; i < n; i = i + 1 {
       print(a[i]);
       fputc(' ', stdout);
    }
    return 0;
}
