
# BZip3

A harder, better, faster and stronger spiritual successor to BZip2. Features higher compression ratios and better performance thanks to a order-2 context mixing entropy coder and fast Burrows-Wheeler transform code making use of suffix arrays.

No stability guarantees yet.

## Installation

```
make all && sudo make install
```

To set the installation directory, use e.g. `sudo make install PREFIX=/usr`.
