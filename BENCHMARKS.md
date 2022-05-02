
Note: These benchmarks are rarely updated.

[Testing corpus](https://github.com/kspalaiologos/bzip3/releases/download/corpus/corpus.7z)

```
17836 bee_movie.txt.bz3
18109 bzip2/bee_movie.txt.bz2

496851 cantrbry.tar.bz3
570856 bzip2/cantrbry.tar.bz2

874691 calgary.tar.bz3
891321 bzip2/calgary.tar.bz2

23036919 enwik8.bz3
29008758 bzip2/enwik8.bz2

48354432 silesia.tar.bz3
54538771 bzip2/silesia.tar.bz2

6639873 lisp.mb.bz3
13462295 bzip2/lisp.mb.bz2

132539016 linux.tar.bz3
157810434 bzip2/linux.tar.bz2
```

## Benchmark on the Calgary corpus

Results:

```
% wc -c corpus/calgary.tar.bz2 corpus/calgary.tar.lzma corpus/calgary.tar.gz corpus/calgary.tar
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

Benchmark 3: ./bzip3 -e -b 3 corpus/calgary.tar corpus/calgary.tar.bz3
  Time (mean ± σ):     286.0 ms ±   4.8 ms    [User: 280.3 ms, System: 4.5 ms]
  Range (min … max):   280.1 ms … 298.9 ms    30 runs

Benchmark 4: bzip2 -9 -k -f corpus/calgary.tar
  Time (mean ± σ):     172.9 ms ±   2.4 ms    [User: 168.4 ms, System: 4.4 ms]
  Range (min … max):   169.5 ms … 179.4 ms    30 runs
```

Memory usage (as reported by `zsh`'s `time`):

```
bzip2 8M memory
bzip3 17M memory
lzma 95M memory
gzip 5M memory
```

## Benchmark on the Linux kernel

```
bzip3 -e -b 32 linux.tar linux.tar.bz3  104.71s user 0.41s system 99% cpu 192M memory 1:45.16 total
bzip2 -9 -k linux.tar  61.23s user 0.35s system 99% cpu 8M memory 1:01.58 total
gzip -9 -k linux.tar  43.08s user 0.35s system 99% cpu 4M memory 43.435 total
lzma -9 -k linux.tar  397.30s user 0.90s system 99% cpu 675M memory 6:38.28 total
```

```
wc -c linux.tar*
1215221760 linux.tar
 157810434 linux.tar.bz2
 208100532 linux.tar.gz
 125725455 linux.tar.lzma
```

