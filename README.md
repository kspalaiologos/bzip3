
# BZip3

A better, faster and stronger spiritual successor to BZip2. Features higher compression ratios and better performance thanks to a order-0 context mixing entropy coder, a fast Burrows-Wheeler transform code making use of suffix arrays and a RLE with Lempel Ziv+Prediction pass based on LZ77-style string matching and PPM-style context modeling.

No stability guarantees yet.

## Installation

```
./configure && make all && sudo make install
```

## Benchmarks

![visualisation of the benchmarks](etc/benchmark.png)

Check etc/BENCHMARKS.md for more results.

## Thanks

- Ilya Grebnov for his `libsais` library used for BWT construction in BZip3 and the LZP encoder which I had used as a reference implementation to improve myself.
