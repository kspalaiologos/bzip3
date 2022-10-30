
# Low level API bzip3 block format.

Each chunk starts with the _new_ size - a 32-bit integer signifying the _compressed_ size of the block, and the _old_ size - a 32-bit integer signifying the _decompressed_ size. Then, a sequence of bzip3-compressed data follows. CRC32 checking is left up to libbz3.

If the chunk is smaller than 64 bytes, then compression is not attempted. Instead, the content is prepended with the 32-bit CRC32 checksum and a 0xFFFFFFFF literal.

Otherwise, the chunk starts with the 32-bit CRC32 checksum value, the Burrows-Wheeler transform permutation index and the compression _model_ - a 8-bit value specifying the compression preset used. As such:

- 2-s bit set in the _model_ - LZP was used and the 32-bit size is prepended to the block.
- 4-s bit set in the _model_ - RLE was used and the 32-bit size is prepended to the block.
- No other bit can be set in the _model_.

The size of libbz3's block header can be calculated using the formula `popcnt(model) * 4 + 9`.
