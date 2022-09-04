## Windows

Cross-compiling Windows binaries is supported:

```console
# For x86_64 (64bit)
$ ./configure CC=x86_64-w64-mingw32-gcc --host x86_64-w64-mingw32
$ make

# For i686 (32bit)
$ ./configure CC=i866-w64-mingw32-gcc --host i686-w64-mingw32
$ make
```

If building a dynamic library is desired, change `PUBLIC_API` in `include/common.h` to `__declspec(dllexport)` or `__attribute__((visibility("default")))` depending on your compiler.

## M1 MacOS

Clang doesn't support `-march=native -mtune=native`, so you should remove them.
