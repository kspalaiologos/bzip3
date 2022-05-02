
# BZip3

A harder, better, faster and stronger spiritual successor to BZip2. Features higher compression ratios and better performance thanks to a order-2 context mixing entropy coder and fast Burrows-Wheeler transform code making use of suffix arrays.

Work-In-Progress. For the time being, don't expect the trunk to be compatible with previous commits.

[Testing corpus](https://github.com/kspalaiologos/bzip3/releases/download/corpus/corpus.7z)

```
496847 cantrbry.tar.bz3
570856 bzip2/cantrbry.tar.bz2

874687 calgary.tar.bz3
891321 bzip2/calgary.tar.bz2

28442351 silesia2.tar.bz3
30128327 bzip2/silesia2.tar.bz2

24790019 enwik8.bz3
29008758 bzip2/enwik8.bz2

21854321 silesia1.tar.bz3
24462553 bzip2/silesia1.tar.bz2

6835103 lisp.mb.bz3
13462295 bzip2/lisp.mb.bz2

138366523 linux.tar.bz3
157810434 bzip2/linux.tar.bz2
```
