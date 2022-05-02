
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

## Benchmark on the Calgary corpus

Results:

```
% wc -c corpus/calgary.tar.bz3 corpus/calgary.tar.bz2 corpus/calgary.tar.lzma corpus/calgary.tar.gz corpus/calgary.tar
 874691 corpus/calgary.tar.bz3
 891321 corpus/calgary.tar.bz2
 853112 corpus/calgary.tar.lzma
1062584 corpus/calgary.tar.gz
3265536 corpus/calgary.tar
```

Performance:

```
Benchmark 1: gzip -9 -k -f corpus/calgary.tar
  Time (mean ± σ):     224.3 ms ±   2.6 ms    [User: 221.4 ms, System: 2.5 ms]
  Range (min … max):   219.9 ms … 230.9 ms    30 runs

Benchmark 2: lzma -9 -k -f corpus/calgary.tar
  Time (mean ± σ):     787.9 ms ±   9.6 ms    [User: 753.6 ms, System: 33.7 ms]
  Range (min … max):   764.8 ms … 813.1 ms    30 runs

Benchmark 3: ./bzip3 -e -b 4 corpus/calgary.tar corpus/calgary.tar.bz3
  Time (mean ± σ):     286.0 ms ±   4.8 ms    [User: 280.3 ms, System: 4.5 ms]
  Range (min … max):   280.1 ms … 298.9 ms    30 runs

Benchmark 4: bzip2 -9 -k -f corpus/calgary.tar
  Time (mean ± σ):     172.9 ms ±   2.4 ms    [User: 168.4 ms, System: 4.4 ms]
  Range (min … max):   169.5 ms … 179.4 ms    30 runs
```

Memory usage (as reported by `zsh`'s `time`):

```
bzip2 8M memory
bzip3 21M memory
lzma 95M memory
gzip 5M memory
```
