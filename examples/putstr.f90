import fputc(c: char, f: FILE): void
import fputs(s: char[100], f: FILE): void
import stdout: FILE

def main(i:int) : int {
   g: char[10];
   g = "abba";
   # print a character to stdout
   fputs[g, stdout];
   fputc['z', stdout];
   return 0;
}

