#!/usr/bin/env bash

set -euo pipefail

mkdir -p results

# strace -c measures syscall counts and percentages per benchmark scenario.
strace -c -o results/baseline.strace.txt \
  ./build/bench_io --mode=baseline --size-mb=50 --output=results/plain_50mb.txt

strace -c -o results/compressed_write.strace.txt \
  ./build/bench_io --mode=compressed-write --size-mb=50 --output=results/write_50mb.ceio

strace -c -o results/compressed_mmap.strace.txt \
  ./build/bench_io --mode=compressed-mmap --size-mb=50 --output=results/mmap_50mb.ceio

# /usr/bin/time -v reports real/user/sys time plus memory-related statistics.
/usr/bin/time -v -o results/baseline.time.txt \
  ./build/bench_io --mode=baseline --size-mb=50 --output=results/plain_50mb.txt

/usr/bin/time -v -o results/compressed_write.time.txt \
  ./build/bench_io --mode=compressed-write --size-mb=50 --output=results/write_50mb.ceio

/usr/bin/time -v -o results/compressed_mmap.time.txt \
  ./build/bench_io --mode=compressed-mmap --size-mb=50 --output=results/mmap_50mb.ceio
