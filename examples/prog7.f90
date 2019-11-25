# radix sort
import fflush(f: FILE): void
import fputc(c: char, f: FILE): void
import fputs(s: char[100], f: FILE): void
import sqrtl(i: int): int
import readint64(): int
import stdout: FILE

#  ---
#  IX: 0 1 2 3
#  A:  4   3x   3y  5 
#  O:  3x  3y   4   5 (we need stable sorts)
#  ---
#  hist[0] = 0
#  hist[1] = 0
#  hist[2] = 0
#  hist[3] = 2
#  hist[4] = 3
#  hist[5] = 4
#  ---
#  out[hist[4] - 1] = arr[0] = 4
#  out[hist[3x] - 1] = arr[3x]    out[1] = 3x
#  out[hist[3y] - 1] = arr[3y]    out[0] = 3y
#  Order will be 3y 3x. 
#  So we need to traverse arr from the _reverse_ order to maintain stability.
#  ---

def maxarr(n: int, xs: int[n]): int {
  m : int = xs[0];

  for i: int = 1; i < n; i = i + 1 {
    if xs[i] > m {
      m  = xs[i];
    }
  }
  return m;
}


# return 10^x
def pow10(x: int): int {
  if (x == 0) { return 1; }
  return 10 * pow10(x - 1);
}

# get the ix-th digit of a
def digit(a: int, ix:int):int {
  return (a / pow10(ix)) % 10;
}

# sort the array of 'xs' by the `pos`th digit in out.
def countsort(n: int, xs: int[n], pos: int, out: int[n]): void {
    # create an array that can store enough data
    counts: int [10];

    for i: int = 0; i < 10; i = i + 1 {
        counts[i] = 0;
    }

    for i: int = 0; i < n; i = i + 1 {
       counts[digit(xs[i], pos)] = counts[digit(xs[i], pos)] + 1;
    }

    hist: int[10];
    hist[0] = counts[0];

    for i: int = 1; i < 10; i = i + 1 {
        hist[i] = hist[i-1] + counts[i];
    }

    for i:int = n-1; i >= 0; i = i - 1 {
        out[hist[digit(xs[i], pos)] - 1] = xs[i];
        hist[digit(xs[i], pos)] = hist[digit(xs[i], pos)] - 1;

    }
}

def radixsort(n: int, xs: int[n], ys: int[n]): void {
  m: int = maxarr(n, xs);
  
  i : int = 0;
  while (m / pow10(i)) > 0 {
     countsort(n, xs, i, ys);
     i = i + 1;

     ### =====
     ### fputs("| i: ", stdout);
     ### print(i);
     ### fputs(" |", stdout);
     ### for j: int = 0; j < n; j = j + 1 {
     ###     fputc(' ', stdout);
     ###     print(ys[j]);
     ### }
     ### fputc('\n', stdout);
     ### fflush(stdout);
     ### =====

     xs = ys;
  }
}


def main() : int {
    n: int = 0;
    n = readint64();
    a: int[n];
    sorted: int[n];

    for i: int = 0; i < n; i = i + 1 {
       a[i] = readint64();
    }

    radixsort(n, a, sorted);

    for i: int = 0; i < n; i = i + 1 {
       print(sorted[i]);
       fputc(' ', stdout);
    }
    return 0;
}
