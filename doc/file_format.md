
# The bzip3 file format

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

After the file header, the bzip3-compressed file contains a series of independent blocks compressed using the low level API.