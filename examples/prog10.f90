import fputc(c: char, f: FILE): void
import fputs(s: char[100], f: FILE): void
import sqrtl(i: int): int
import readint64(): int
import assert(b: bool, s:char[100]): int
import stdout: FILE


def matmul(n: int, m: int, o: int, a: int[n, o], b :int[o, m], c:int[n, m]): void {
    for i: int = 0; i < n; i = i + 1 {
        for j : int = 0; j < m; j = j + 1 {
            c[i, j] = 0;
            for k : int = 0; k < o; k = k + 1 {
                c[i, j] = c[i, j] + a[i, k] * b[k, j];
            }
        }
    }
}

def main() : int {
    n: int = readint64();
    m: int = readint64();
    o: int = readint64();

    a: int[n, o];
    b: int[o, m];
    c: int[n, m];
    
    for i: int = 0; i < n; i = i + 1 {
        for j : int = 0; j < o; j = j + 1 {
            a[i, j] = readint64();
        }
    }

    for i: int = 0; i < o; i = i + 1 {
        for j : int = 0; j < m; j = j + 1 {
            b[i, j] = readint64();
        }
    }

    matmul(n, m, o, a, b, c);

    for i: int = 0; i < n; i = i + 1 {
        for j : int = 0; j < m; j = j + 1 {
            print(c[i, j]);
            fputc(' ', stdout);
        }
        fputc('\n', stdout);
    }

    return 0;
}
