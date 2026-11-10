# Experiments

## Small executable example

`amskt_demo` uses `N=256`, `w=64`, `m=64`, 10% Bernoulli group activity, and 2000 distinct synthetic entities per active group. The input generator and release seeds are fixed. Duplicate memberships are inserted into both partitions to check duplicate elimination during merging.

The output reports nonempty-group AARE for both release settings and inactive-group MAE after clipping for private estimates. Empty groups remain in the complete release vector. This example is intended for a quick build and usage check.

## Synthetic-10K runner

Enable the optional target when configuring CMake:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DAMSKT_BUILD_EXPERIMENTS=ON
cmake --build build --config Release --parallel 4
./build/amskt_compare compare experiments/config/synthetic_10k_one_trial.csv outputs/synthetic_10k 1 1
```

The final arguments select the number of trial indices to include and the number of worker threads. The supplied configuration includes trial 0 only. It retains all four memory points and activity values of 1%, 5%, 10%, and 20%. The 5% activity point at 2 MiB is shared with the memory scan rather than generated twice.

The public directory contains 10000 groups. Each active group contains 10000 distinct synthetic entities. Layouts are fixed before the Bernoulli activity draw. The memory scan sets `w=512` and `m=128,256,512,1024`, corresponding to 0.5, 1, 2 and 4 MiB. The runner produces non-private, epsilon = 1, and epsilon = 0.5 results from the same constructed state where applicable.

This workload is much larger than the quick demo. It can take several minutes or longer, depending on the machine. Start with one worker. The programs can write per-group outputs of several megabytes. Generated output stays under the ignored `outputs/` directory.

The supplied seeds are one selected existing trial, not seeds selected for a method ranking. A single run is not sufficient to reproduce the paper's means or confidence intervals. Reproducing those statistics requires the full fixed trial configuration and the corresponding analysis.

## Output files

- `run_i.csv` stores trial-level active AARE, raw and clipped inactive MAE, full-vector WAPE, layout and seed fields.
- `groups_i.csv`, where written by the runner, stores all directory entries and their raw count estimates.
- `*.partial` denotes an incomplete output. Completed trial files are skipped on restart.

Keep input-hash seeds separate from private noise seeds in a real deployment. The stored noise seeds are suitable only for public synthetic experiments.

## Tests

`ctest` runs three programs:

1. `core_properties` checks Hadamard orthogonality, duplicate elimination, merge equivalence, inverse endpoints, decoder consistency and release behavior.
2. `experiment_regression` compares the current engine with its reference state and verifies the main-release and ablation noise/decoder pairing.
3. `synthetic_demo` runs the complete small example.

These are correctness and packaging checks. They do not constitute a proof of differential privacy or a rerun of the complete manuscript experiment matrix.
