set -e
set -o xtrace
./tinyfort $1 > $1.ll
llc $1.ll -o $1.o -filetype=obj
gcc $1.o  -L$(pwd) -lfort -lm -o $1.out
./$1.out
