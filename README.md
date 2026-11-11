# AMSkt

Representative research code for **AMSkt: Accurate and Private Distinct Counting for Multiple Groups**.

AMSkt estimates distinct counts for a fixed public directory of groups. It shares bitmap registers across groups, uses Hadamard assignments to balance sharing, and reconstructs counts from decoded block statistics. Compatible states can be merged before a joint private release.

This repository provides a focused **FMS implementation** and small executable examples. It includes AMSkt, vSkt, rSkt, CMSkt, and preallocated independent FMS under a common logical bitmap budget.

[Implementation details](docs/IMPLEMENTATION.md) · [Experiments](docs/EXPERIMENTS.md)

## Quick start

Requirements are a C++20 compiler and CMake 3.20 or newer. The basic examples do not need Python, downloaded datasets, or external libraries.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 4
ctest --test-dir build -C Release --output-on-failure
./build/amskt_demo
```

On Windows with MinGW, put the compiler's `bin` directory on `PATH` and add `-G "MinGW Makefiles"` when configuring. Executables have the `.exe` suffix. With a multi-configuration generator, use `build/Release/amskt_demo.exe`.

The demo creates synthetic memberships for 256 configured groups, builds and merges two partition-local states, and compares all five methods at a 32 KiB logical bitmap budget and epsilon = 1. It checks that merging gives the same state as direct construction. Estimates are clipped to zero before reporting errors.

To save its small summary:

```bash
./build/amskt_demo outputs/demo.csv
```

The CSV reports nonempty-group AARE and inactive-group MAE. Demo measurements are not paper results.

## Demo Test

```cpp
#include "fms_experiment.hpp"
using namespace amskt_experiment;

FiveSketches left(256, 64, 64, 2026);
FiveSketches right(256, 64, 64, 2026);
left.update(1001, 7);   // entity identifier, zero-based group index
right.update(1001, 7);  // duplicate membership in another partition
right.update(1002, 7);
left.merge(right);
```

`FiveSketches` holds the comparison methods together so that they read identical updates. The AMSkt arrays are `ap` and `ac`. See `examples/demo.cpp` for decoder setup and private release.

## Included

| Path | Contents |
|---|---|
| `include/amskt/fms_experiment.hpp` | Group-salted FMS engine, five methods, merging and private release |
| `examples/demo.cpp` | Small synthetic comparison and merge example |
| `tests/` | Orthogonality, duplicate elimination, merge and decoder checks |
| `experiments/unified_fms.cpp` | Optional Synthetic-10K and component-ablation runner |
| `experiments/config/` | One fixed synthetic trial for a memory and activity sweep |
| `docs/` | Configuration, memory accounting and execution instructions |

The core engine and experiment runner are preserved from the experiment implementation. Build files, examples and documentation provide a smaller entry point. The regression reference under `tests/reference/` is used only by tests.

## Larger synthetic example

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DAMSKT_BUILD_EXPERIMENTS=ON
cmake --build build --config Release --parallel 4
./build/amskt_compare compare experiments/config/synthetic_10k_one_trial.csv outputs/synthetic_10k 1 1
```

This is an optional, larger workload. Read [the experiment guide](docs/EXPERIMENTS.md) before running it. The supplied configuration has one trial. It does not establish the paper's 30-trial confidence intervals.

## Scope and privacy

- This initial code release focuses on FMS. It does not include every FM/LLC implementation or every paper figure.
- Memory budgets count logical bitmap words. Mapping tables, decoder tables, output vectors and process overhead are additional memory.
- The example uses fixed pseudorandom seeds for reproducibility on synthetic data. For a real private release, keep privacy-noise seeds confidential and use a suitably secure noise source. This research implementation is not a hardened privacy service.
- Complete group vectors are reconstructed before metrics are restricted to nonempty groups. Private baseline calibration includes the full public group directory.
- Raw PSID, NLSY79 and KASANDR records, entity-level derived files, manuscripts, reviews, logs and large experiment outputs are not included.

## Citation

The repository accompanies **AMSkt: Accurate and Private Distinct Counting for Multiple Groups**. `CITATION.cff` contains the current title and authors. No publication venue or DOI is assigned here.

## License

A project-wide open-source license has not yet been selected. No MIT, Apache or other reuse license is implied. See [NOTICE.md](NOTICE.md). Repository visibility does not by itself grant a software license.
