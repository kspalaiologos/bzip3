
## Windows

Porting to Windows shouldn't be hard when using Cygwin. Make sure to install `gcc` (because `clang` seems to work poorly on Cygwin), update the Makefile to use `gcc` and update the `CFLAGS` variable - remove `-g3` and `-fPIC`, add `-D_WIN32`. Replace `.so` with `.dll` in the shared library name.

## M1 MacOS

Clang doesn't support `-march=native -mtune=native`, so you should remove them.
