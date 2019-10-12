#!/usr/bin/env bash
set -e
set -o xtrace

for path in $1/prog*.f90; do
    ./test-example.sh $path
done
