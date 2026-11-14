#!/usr/bin/env bash
# Build everything in a Release build and run every benchmark, pinned to two physical cores,
# saving JSON results under build/bench/results/.
#
# usage: bench/run.sh [label] [cpu-list]
#   label     prefix for the result files (default: baseline), e.g. "after-slab-change"
#   cpu-list  CPUs to pin to (default: 2,3). They must be on DIFFERENT physical cores (see `lscpu -e`: CPUs with
#             the same CORE number are hyperthread siblings). The cross-thread queue benchmarks need two cores.
#
# For stable numbers, first: sudo cpupower frequency-set -g performance
# To compare two runs: build/bench/_deps/googlebenchmark-src/tools/compare.py benchmarks <old.json> <new.json>
# (it needs python3 with scipy and numpy)
set -euo pipefail

label="${1:-baseline}"
cpus="${2:-2,3}"
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build="$root/build/bench"
out="$build/results"

cmake -S "$root" -B "$build" -G Ninja -DCMAKE_BUILD_TYPE=Release -DEXCHANGE_BUILD_BENCH=ON -DEXCHANGE_BUILD_TESTS=OFF
cmake --build "$build" --target benchmarks
mkdir -p "$out"

for exe in "$build"/bench/*_bench; do
    name="$(basename "$exe")"
    echo "=== $name (cpus $cpus)"
    taskset -c "$cpus" "$exe" \
        --benchmark_repetitions=10 \
        --benchmark_report_aggregates_only=true \
        --benchmark_out="$out/${label}_${name}.json" \
        --benchmark_out_format=json
done
echo "results in $out"
