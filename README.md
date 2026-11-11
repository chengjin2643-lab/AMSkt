# AMSkt: Accurate and Private Distinct Counting for Multiple Groups

[![Language: C++20. Build: CMake 3.20 or newer.](docs/assets/toolchain.svg)](CMakeLists.txt)

How many different users saw each ad across several websites? A user who sees the same ad on two sites counts **once** for that ad. A user who sees two ads counts **once for each**. AMSkt estimates these counts from compact local sketches, without pooling the view logs.

![Sites build local sketches with a shared layout. A trusted service merges their bitmaps, adds noise to the merged readings, and releases one estimate per group.](docs/assets/amskt-overview.svg)

[🚀 Quickstart](#quickstart) · [🧪 Demo Test](#demo-test) · [🧩 Methods](#methods) · [📊 Workloads](#workloads) · [🎛️ Parameters](#parameters)

## Quickstart

You need a C++20 compiler and CMake 3.20 or newer. The demo generates its own data and uses only the C++ standard library.

```bash
git clone https://github.com/chengjin2643-lab/AMSkt.git
cd AMSkt
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 4
ctest --test-dir build -C Release --output-on-failure
./build/amskt_demo outputs/demo.csv
```

<details>
<summary>Windows executable paths</summary>

With MinGW, put its `bin` directory on `PATH` and add `-G "MinGW Makefiles"` to the configure command. Run `.\build\amskt_demo.exe outputs\demo.csv`. With a multi-configuration generator, the executable is under `build/Release/` instead.

</details>

## Demo Test

**Try a duplicate across two sites.** In the C++ API, `update(entity, group)` takes the entity ID first and a zero-based group index second. Both sites use the same public layout and hash seed.

```cpp
using namespace amskt_experiment;  // from fms_experiment.hpp
FiveSketches site_a(256, 64, 64, 2026), site_b(256, 64, 64, 2026);

site_a.update(1001, 7);
site_b.update(1001, 7);  // same user, same ad: duplicate across sites
site_b.update(1002, 7);  // another user saw ad 7
site_b.update(1001, 8);  // user 1001 also counts for ad 8
site_a.merge(site_b);
```

The true audiences are **2 for ad 7** and **1 for ad 8**. Merging reproduces the sketch built from the union, not the sum of the two sites' counts. The released estimates need not equal these exact counts.

**Read the executable's output.** The [full demo](examples/demo.cpp) runs a larger version with 256 groups. It checks that direct and merged sketches give identical estimates when released with identical noise. Here is the AMSkt row from one run (GCC 15.2, 22 active groups):

```csv
method,active_groups,bitmap_budget_bytes,epsilon,nonprivate_aare,private_aare,inactive_mae
AMSkt,22,32768,1,0.0536650353,0.06082097219,16.42309071
```

Here `private_aare` means an average relative error of about **6.08% on active groups**. For groups whose true count was zero, the mean absolute error was **16.42 users** after clipping. The CSV includes all five methods. These are demo values, not paper averages.

## Methods

All five methods in the [comparison engine](include/amskt/fms_experiment.hpp) receive the same memberships. This release uses 32-bit FMS bitmaps. The budget counts each method's logical bitmap storage, not the total memory of the process running all five.

| Method | How it uses the bitmap budget |
| :--- | :--- |
| **AMSkt** | Groups in the same row use balanced Hadamard assignments. AMSkt decodes paired blocks before combining their signed differences. |
| vSkt | Maps virtual registers onto a shared array and uses a global statistic to correct for background traffic. |
| rSkt | Randomly assigns the two sides of each register pair and uses primary and complementary summaries. |
| CMSkt | Updates a component sketch in each of two hashed layers and takes the smaller estimate. |
| Independent FMS | Reserves a separate bitmap for every group in the public directory, including inactive groups. |

🔒 **Private release.** A trusted service merges local states before adding noise. The protected unit is one entity's membership in one group. The demo uses fixed noise seeds for repeatability, not for protecting live data. See [implementation and noise calibration](docs/IMPLEMENTATION.md).

## Workloads

| Workload | What you can run |
| :--- | :--- |
| **Quick demo** | Two sites, 256 groups, 10% activity probability, and 2,000 distinct entities per active group. No downloads. |
| **Synthetic-10K** | One included trial over 10,000 registered groups. Sweep memory from 0.5 to 4 MiB or activity from 1% to 20%. |
| **PSID, NLSY79, KASANDR** | Real workloads evaluated in the paper. Their records are not distributed in this repository. |

To run the [supplied Synthetic-10K configuration](experiments/config/synthetic_10k_one_trial.csv) from the repository root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DAMSKT_BUILD_EXPERIMENTS=ON
cmake --build build --config Release --parallel 4
./build/amskt_compare compare experiments/config/synthetic_10k_one_trial.csv outputs/synthetic_10k 1 1
```

The final two arguments select **one trial index** and **one worker**. Open `outputs/synthetic_10k/run_0.csv` for the first configuration's results. The supplied trial illustrates the experiment; it does not reproduce the paper's 30-trial confidence intervals. See the [experiment guide](docs/EXPERIMENTS.md) for output columns and tests.

## Parameters

The small demo fixes these values in [examples/demo.cpp](examples/demo.cpp); they are not command-line flags.

| Setting | Demo value | Meaning |
| :--- | ---: | :--- |
| `N` | 256 | Number of group keys in the public directory |
| `w` | 64 | Physical rows in each AMSkt array |
| `m` | 64 | Bitmap registers per row; the two arrays use `8 × w × m` bytes in total |
| `epsilon` | 1.0 | Privacy budget for one joint count release |
| Output path | optional | Pass a CSV path to `amskt_demo` to save the printed summary |

## Citation and usage

Use [CITATION.cff](CITATION.cff) for the paper title and author list. No project-wide open-source license has been assigned; see the [distribution notice](NOTICE.md).
