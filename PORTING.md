
## Windows

Porting to Windows shouldn't be hard when using Cygwin. Make sure that you have installed all the relevant packages (clang, 32-bit gcc if required, make, git).
There are a few rudimentary fixes that need to be built BZip3 on Windows, like with every Linux software. The first tweak is removing `-fPIC` from the Makefile, as Windows doesn't need it. Next, you will need to open `include\common.h`, find the `#define PUBLIC_API` line and define `PUBLIC_API` as `__declspec(dllexport)`.
The final tweak is adjusting the C compiler if needed (the `CC` variable in the Makefile), fixing extensions to match Windows expectations (`bzip3` => `bzip3.exe`, `libbzip3.so` => `libbzip3.dll`). 
