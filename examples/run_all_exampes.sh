set -e
set -o xtrace
# expects tinyfort to be here
TINYFORT=../build/tinyfort

for f in prog*.f90; do
    $TINYFORT $f > build/$f.ll
    # use optimisation so we do not run out of stack space.
    opt build/$f.ll -S -O3 -o build/$f.o3.ll
    llc build/$f.o3.ll -o build/$f.o -filetype=obj
    gcc build/$f.o  -L$(pwd) -L../build -lfort -lm -o build/$f.out
done
