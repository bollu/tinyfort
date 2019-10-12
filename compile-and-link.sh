set -e
set -o xtrace
./tinyfort $1 > $1.ll
# use optimisation so we do not run out of stack space.
opt $1.ll -S -O3 -o $1.o3.ll
llc $1.o3.ll -o $1.o -filetype=obj
gcc $1.o  -L$(pwd) -lfort -lm -o $1.out
