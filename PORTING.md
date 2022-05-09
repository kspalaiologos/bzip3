
## Windows

Crosscompiling to Windows is supported:

```bash
CC=i686-w64-mingw32-gcc make
# or
CC=x86_64-w64-mingw32-gcc make
```

## M1 MacOS

Clang doesn't support `-march=native -mtune=native`, so you should remove them.
