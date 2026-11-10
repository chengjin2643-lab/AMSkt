# Source provenance

The following files were copied byte-for-byte from the FMS experiment snapshot `revision_experiments_20261102`. The core algorithm was not changed during packaging.

| Repository file | SHA-256 |
|---|---|
| `include/amskt/fms_experiment.hpp` | `7fe5985dd68b8bece2aec5ec7019a56665989e11e3d4decba87381794b6f1bd6` |
| `tests/reference/fms_original.hpp` | `1d4d37303075346b9b8a27b84070fe34528e60a9ddad306600fa236b68fbfb00` |
| `tests/fms_checks.cpp` | `e877c89cadee6b178ecdd120d622c0d71e62fec22add4e43968a75233c22e23d` |
| `experiments/unified_fms.cpp` | `9e28c76c43270b412f48b793c79f2c4688fc682e9bb4c8aa7cccdc58fb9cc07e` |

The one-trial synthetic configuration matches trial 0 of the saved experiment configuration. All seven supplied tasks were run with the packaged runner. Their trial-level output fields match the saved trial 0 results exactly on the tested MinGW toolchain.

Packaging adds CMake targets, a small demo, additional tests, README files, and implementation instructions. The regression reference is test-only.

Verification used GCC 15.2.0 and CMake on Windows with MinGW. All three CTest checks passed. No Linux or MSVC runtime result is claimed.

This is a representative FMS source subset, not the complete manuscript artifact. It excludes real entity-level data, manuscript sources, reviews, credentials, binaries, and generated results. A project-wide software license is not assigned.
