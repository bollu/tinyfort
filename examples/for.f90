import fputc(c: char, f: FILE): void
#  import fputs(s: char[100], f: FILE): void
import stdout: FILE
def main(i:int) : int {
    for i : int = 0; i <= 10; i = i + 1 {
      print(i);
      fputc(' ', stdout);
    }
    fputc('\n', stdout);

    for i : int = 0; i <= 10; i = i + 1 {
        for j : int = 0; j <= 10; j = j + 1 {
        print(j);
        fputc(' ', stdout);
        }
      
    fputc('\n', stdout);
    }
    return 0;
}
