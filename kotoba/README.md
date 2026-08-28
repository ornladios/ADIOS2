<!--
SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors

SPDX-License-Identifier: Apache-2.0
-->

# Kotoba v1 ADIOS2/BP binding

Honest v1 only: **magic / header**. This tree is a first-class sibling of
`bindings/{C,CXX,Fortran,Matlab,Python}` **on this fork**. It is not a
replacement for the C++ library and it is not robotics-ready.

Owner constraint: magic/header or one record. No I/O engine, no BP writer,
no timestep/query.

The module compiles with [Kotoba](https://github.com/kotoba-lang/kotoba) CLI
**0.7.2** to `wasm32-kotoba-v1` under the `i64-v1` value profile: no FFI, no
IEEE floats, no vector or externref ABI.

## What this implements

Enough of the on-disk 64-byte BP5 Index Table header to identify the format.
Field offsets match `BP5IndexTableHeader` in
`source/adios2/engine/bp5/BP5Engine.h` (layout source only).

| Offset | Fixture | Meaning |
| ------ | ------- | ------- |
| 0-7 | `ADIOS-BP` | File magic |
| 36 | `0` | Little-endian (`0` little, `1` big) |
| 37 | `5` | BP major version |
| 38 | `2` | BP5 minor version |
| 39 | `0` | Header active-flag byte |

The fixture is `fixtures/bp5-index-header.bin` (exactly 64 bytes).
`adios2.kotoba` reads those identification fields and packs them as `110520`:

- `1` magic `ADIOS-BP`
- `1` little-endian
- `05` BP version
- `2` BP minor
- `0` active-flag byte

## What this is not

- Not an I/O engine.
- Not a BP writer.
- Not timestep, step, or query support.
- Not a reader for `md.0`, `mmd.0`, `data.*`, variables, or attributes.
- Not FFS / MetaMeta decode.
- Not a replacement for the C, C++, Fortran, Matlab, or Python bindings.
- Not robotics-ready.

This is not a claim that Kotoba can open production ADIOS2 datasets.

## Checks

`checks.sh` downloads Kotoba 0.7.2 (or uses `KOTOBA` / `KOTOBA_BIN`), compiles
`adios2.kotoba` to wasm, and requires a real compiler receipt:

- `value-profile` is `i64-v1`
- target is `wasm32-kotoba-v1`
- `value-abi` is `direct-v1`
- `wasm-features` is empty
- the artifact starts with wasm magic and carries `wasm32-kotoba-v1`

It then runs the module and requires runtime value `110520`. The script fails
if the fixture, module comment, or field literals drift. It does not invent a
pass.

```sh
bash kotoba/checks.sh
```

## Upstream

This binding lives on `kotoba-lang/ADIOS2`. It is not an `ornladios/ADIOS2`
release surface. Do not open a pull request to `ornladios/ADIOS2` from this
tree.

Fork operator: [awai.network](https://awai.network) / Ryo Awai.
