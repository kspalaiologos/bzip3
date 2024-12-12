
# High level API bzip3 frame format.

The bzip3 frame format is a concatenation of bzip3-compressed blocks. It's used exclusively by the `bz3_compress` and `bz3_decompress` functions and will not work with the command-line tool or low level functions. Each frame start with the ASCII "BZ3v1" signature, followed by the 32-bit maximum block size in bytes. After the 9 byte header, a sequence of independent blocks encoded using the low level API follows.
