set -e
set -o xtrace
./compile-and-link.sh $1
./$1.out $2 < $1.input | tee $1.output
diff $1.output $1.output.golden --ignore-space-change
