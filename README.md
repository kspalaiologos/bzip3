
# BZip3

A better, faster and stronger spiritual successor to BZip2. Features higher compression ratios and better performance thanks to a order-0 context mixing entropy coder and fast Burrows-Wheeler transform code making use of suffix arrays.

No stability guarantees yet.

## Installation

```
./configure && make all && sudo make install
```

## TODO

- Avoid a memory copy in `bz3_encode_block`/`bz3_decode_block`.
- "Extreme" preset which automatically picks the best options at the expense of the runtime.
- Further performance optimisations.
- Thorough memory safety testing.
