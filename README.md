# tinyfort

a minimal FORTran.

# Build instructions

Use nix for reproducible builds. Run:
```
$ nix-shell # for reproducible builds with exact dependencies setup automatically
$ mkdir build && cd build && cmake ../ && make
$ ln -s ../compile-and-link.sh compile-and-link.sh
$ compile-and-link.sh ../examples/prog1.f90
```

### Interpreter stuff
- prog11, prog12 have a bug related to arguments

### Programs
- prog10 needs 3 arguments: (n, m, o)

