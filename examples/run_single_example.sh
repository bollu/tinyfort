set -e
set -o xtrace
# expects tinyfort to be here
TINYFORT=../build/tinyfort

$TINYFORT $1 > build/$1.ll
# use optimisation so we do not run out of stack space.
opt build/$1.ll -S -O3 -o build/$1.o3.ll
llc build/$1.o3.ll -o build/$1.o -filetype=obj
gcc build/$1.o  -L$(pwd) -L../build -lfort -lm -o build/$1.out
build/$1.out
