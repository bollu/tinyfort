import fputc(c: char, f: FILE): void
import fputs(s: char[100], f: FILE): void
import sqrtl(i: int): int
import readint64(): int
import assert(b: bool, s:char[100]): int
import stdout: FILE

def mergesort(len: int, xs: int[len]): int {
    if (len <= 1) { 
        return 0;
    }
    
    # llen + rlen = len
    llen : int = len / 2;
    rlen : int = len - llen;

    left : int[llen];
    right: int[rlen];

    # copy data
    i : int = 0;
    while i < llen {
        left[i] = xs[i];
        i = i + 1;
    }

    i = 0;
    while i < rlen {
        right[i] = xs[llen + i];
        i = i + 1;
    }

    # recursively sort
    mergesort(llen, left);
    mergesort(rlen, right);

    # now merge
    lix : int = 0;
    rix : int = 0;
    arrix : int = 0;

    while lix < llen && rix < rlen {
        if left[lix] > right[rix]  {
            xs[arrix] = right[rix];
            rix = rix + 1;
            arrix = arrix + 1;
        } else {
            xs[arrix] = left[lix];
            lix = lix + 1;
            arrix = arrix + 1;
        }
    }

    while lix < llen {
        xs[arrix] = left[lix]; lix = lix + 1; arrix = arrix + 1;
    }

    while rix < rlen {
        xs[arrix] = right[rix]; rix = rix + 1; arrix = arrix + 1;
    }

    # TODO: allow return from void functions.
    return 0;
}


def main() : int {
    n: int = 0;
    n = readint64();
    a: int[n];

    for i: int = 0; i < n; i = i + 1 {
       a[i] = readint64();
    }

    mergesort(n, a);

    for i: int = 0; i < n; i = i + 1 {
       print(a[i]);
       fputc(' ', stdout);
    }
    return 0;
}
