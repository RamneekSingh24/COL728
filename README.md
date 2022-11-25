# cc
C compiler

The C language grammar (c.y and c.l files) have been taken from:

http://www.quut.com/c/ANSI-C-grammar-y-2011.html

## Requirements
bison version >= 3.2

## Usage
You may want to set `LLVM_CONFIG` and `BISON` variables in the top of Makefile.

By default they are set to:

```make
LLVM_CONFIG=llvm-config
BISON=bison
```

```bash
make # Builds the compiler
./cc examples/test2.c # Emits the LLVM IR code or prints errors if any

# To run the code, use the lli (only if no errors are printed)
./cc examples/test2.c > test2.lli
lli test2.lli
echo $? # prints 66 for the given test2

make clean # cleans up the build
```