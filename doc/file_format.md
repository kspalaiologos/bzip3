
# The bzip3 file format

## The header

Each bzip3-compressed file starts with the marker `BZ3v1`. After the signature, the compressor encodes a 32-bit number signifying the maximum block size in bytes in the file. As such, no block after decompression in the stream can exceed it. The maximum block size must be between 65KiB and 511MiB.

The following functions are used for serialising all 32-bit numbers to the archive:

```c
static s32 read_neutral_s32(u8 * data) {
    return ((u32)data[0]) | (((u32)data[1]) << 8) | (((u32)data[2]) << 16) | (((u32)data[3]) << 24);
}

static void write_neutral_s32(u8 * data, s32 value) {
    data[0] = value & 0xFF;
    data[1] = (value >> 8) & 0xFF;
    data[2] = (value >> 16) & 0xFF;
    data[3] = (value >> 24) & 0xFF;
}
```

## The chunks

After the header, the file may contain an unlimited amount of chunks. Each chunk starts with the _new_ size - a 32-bit integer signifying the _compressed_ size of the block, and the _old_ size - a 32-bit integer signifying the _decompressed_ size. Then, a sequence of bzip3-compressed data follows. CRC32 checking is left up to libbz3.

If the chunk is smaller than 64 bytes, then compression is not attempted. Instead, the content is prepended with the 32-bit CRC32 checksum and a 0xFFFFFFFF literal.

Otherwise, the chunk starts with the 32-bit CRC32 checksum value, the Burrows-Wheeler transform permutation index and the compression _model_ - a 8-bit value specifying the compression preset used. As such:

- 2-s bit set in the _model_ - LZP was used and the 32-bit size is prepended to the block.
- 4-s bit set in the _model_ - RLE was used and the 32-bit size is prepended to the block.
- No other bit can be set in the _model_.

The size of libbz3's block header can be calculated using the formula `popcnt(model) * 4 + 1`.
