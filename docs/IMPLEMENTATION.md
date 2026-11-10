# Implementation

## Public layout

The public group directory has `N` entries, indexed from zero. AMSkt uses two arrays with `w` rows and `m` 32-bit bitmap words per row. Its logical bitmap storage is `8*w*m` bytes.

For `q = ceil(N/w)`, the FMS implementation sets `K = bit_ceil(q)` and `s = m/K`. Each block contains `s` words and uses one assignment sign. The configuration requires positive dimensions, `K <= m`, and `m % K == 0`. For these comparison helpers, `2*w` must also fit in a 16-bit mapping entry. The directory and layout are fixed before records arrive.

Updates hash the entity together with a salt derived from its group index. Duplicate copies of the same entity-group membership produce the same update. FM-style ranks at or beyond 31 use the last bit of a 32-bit word.

## Decoding and release

`Inverse` evaluates the finite expected zero-bit count and numerically inverts it using a continuous lookup table. The public count cap is `1e8`, and each table has 200001 nodes. Input statistics are clipped to the valid zero-count range before inversion.

Create one `Inverse` for each register count required by `release`, then insert it into the `Inverses` map. The required keys are `s`, `m`, `vrows*m`, `R`, and `cm`. Duplicate keys can share a table.

`release` returns raw non-private and private reconstructions for all configured groups. Apply `max(0, estimate)` to final outputs before reporting the final counts. It is important not to remove inactive groups before sampling release noise.

For the complete released statistic vectors, the Laplace scales used here are:

| Method | Scale |
|---|---|
| AMSkt | `1/epsilon` per physical block statistic |
| vSkt | `(vM+1)/epsilon`, including its global statistic |
| rSkt | `rM/epsilon` |
| CMSkt | `2/epsilon` for its two layers |
| Independent FMS | `1/epsilon` per group statistic |

The sharing bounds are computed over the full public directory. Independent FMS also uses a joint calibration. Its noise is not multiplied by `N`.

The release code uses deterministic pseudorandom generators for experiment repeatability. A production privacy service needs an appropriate private randomness source and must not disclose noise seeds. The code does not implement a privacy accountant for repeated releases.

## Merging

All partitions must use identical public layouts and hash seeds. `merge` applies bitwise OR to matching bitmap words. It rejects incompatible configurations. The merged state equals construction from the union of the input memberships, including duplicate elimination.

## Comparison memory

`FiveSketches` maintains all five methods at once for paired experiments. Each method has its own bitmap allocation. The process therefore uses more than one method's nominal budget.

- AMSkt and rSkt each use two `w*m` arrays.
- vSkt uses one `2*w*m` array.
- CMSkt uses two `cw*cm` arrays. In the synthetic sweep, `cm=128` and `cw=512*m/128`.
- Independent FMS reserves `R=floor((8*w*m)/(4*N))` words for every directory entry.

Decoder tables, mapping tables, output vectors and standard-library overhead are not counted in these logical bitmap budgets.
