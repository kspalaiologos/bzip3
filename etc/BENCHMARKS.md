
Note: These benchmarks are rarely updated.

[Testing corpus](https://github.com/kspalaiologos/bzip3/releases/download/corpus/corpus.7z)

```
17257 bee_movie.txt.bz3
18109 bzip2/bee_movie.txt.bz2
55315 bee_movie.txt

72319 bzip3-master.tar.bz3
77575 bzip2/bzip3-master.tar.bz2
501760 bzip3-master.tar

256680 lua-5.4.4.tar.bz3
285841 bzip2/lua-5.4.4.tar.bz2
1361920 lua-5.4.4.tar

468273 cantrbry.tar.bz3
570856 bzip2/cantrbry.tar.bz2
2821120 cantrbry.tar

807959 calgary.tar.bz3
891321 bzip2/calgary.tar.bz2
3265536 calgary.tar

2347279 decoda.tar.bz3
2580600 bzip2/decoda.tar.bz2
6154240 decoda.tar

1229841 shakespeare.txt.bz3
1479261 bzip2/shakespeare.txt.bz2
5458199 shakespeare.txt

2052611 2b2t_signs.txt.bz3
2388597 bzip2/2b2t_signs.txt.bz2
9635520 2b2t_signs.txt

41187302 audio.tar.bz3
95526840 bzip2/audio.tar.bz2
115742720 audio.tar

12753535 chinese.txt.bz3
17952181 bzip2/chinese.txt.bz2
79912971 chinese.txt

22677653 enwik8.bz3
29008758 bzip2/enwik8.bz2
100000000 enwik8

47227869 silesia.tar.bz3
54538771 bzip2/silesia.tar.bz2
211968000 silesia.tar

8437744 lisp.mb.bz3
13462295 bzip2/lisp.mb.bz2
371331415 lisp.mb

83624653 gcc.tar.bz3
109065903 bzip2/gcc.tar.bz2
824309760 gcc.tar

157642135 dna.tar.bz3
180075480 bzip2/dna.tar.bz2
685619200 dna.tar

129023226 linux.tar.bz3
157810434 bzip2/linux.tar.bz2
1215221760 linux.tar

406343481 Windows NT 4.0.vmdk.bz3
437184515 bzip2/Windows NT 4.0.vmdk.bz2
804192256 Windows NT 4.0.vmdk
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

Benchmark 3: bzip3 -e -b 3 corpus/calgary.tar corpus/calgary.tar.bz3
  Time (mean ± σ):     265.3 ms ±   1.8 ms    [User: 257.6 ms, System: 5.9 ms]
  Range (min … max):   262.5 ms … 269.0 ms    11 runs

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
bzip3 -e -b 16 corpus/linux.tar corpus/linux.tar.bz3  78.14s user 0.35s system 99% cpu 90M memory 1:18.51 total
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

