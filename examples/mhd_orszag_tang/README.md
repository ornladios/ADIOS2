.. SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
..
.. SPDX-License-Identifier: Apache-2.0

# 2D Orszag-Tang MHD Example

This example runs a 2D uniform-grid ideal-MHD Orszag-Tang vortex solver and
writes simulation fields and diagnostics with ADIOS2.

The solver supports:

- solver variants: `hll`, `rusanov`, `muscl_hll`, `muscl_rusanov`
- optional MPI domain decomposition in the `y` direction when ADIOS2 is built
  with MPI
- ADIOS2 BP output
- output cadence control by step and/or simulation time

Each output step writes 2D fields:

- `rho`
- `pressure`
- `vx`, `vy`, `vz`
- `bx`, `by`, `bz`
- `speed`
- `current_z`

Optional outputs:

- `psi` with `--psi`
- `mx`, `my`, `mz` with `--m`
- `E` with `--E`

Rank 0 writes scalar diagnostics per step:

- `step`, `time`
- `mass`
- `kinetic_energy`, `magnetic_energy`, `internal_energy`, `total_energy`
- `mean_pressure`
- `max_speed`
- `current_abs_max`, `current_rms`
- `divb_abs_max`, `divb_l2`

## Build

Configure ADIOS2 with examples enabled, then build the target:

```bash
cmake --build <adios2-build-dir> --target mhd_orszag_tang
```

The executable is created as:

```bash
<adios2-build-dir>/bin/mhd_orszag_tang
```

## Run

Small serial run:

```bash
<adios2-build-dir>/bin/mhd_orszag_tang \
  --nx 64 --ny 64 \
  --solver muscl_hll \
  --tfinal 0.1 --cfl 0.3 \
  --output-dir runs/ot_muscl \
  --output-every-steps 5
```

MPI run, if ADIOS2 was built with MPI:

```bash
mpirun -n 2 <adios2-build-dir>/bin/mhd_orszag_tang \
  --nx 64 --ny 64 \
  --solver muscl_hll \
  --tfinal 0.1 --cfl 0.3 \
  --output-dir runs/ot_muscl_mpi \
  --output-every-steps 5
```

Each run output directory contains:

- `output.bp`
- `input_parameters.txt`

## Important Controls

- `--solver`: `rusanov`, `hll`, `muscl_hll`, `muscl_rusanov`
- `--output-dir <path>`: writes `<path>/output.bp` and
  `<path>/input_parameters.txt`
- `--output <path>`: writes an explicit BP output path
- `--engine <name>`: ADIOS2 engine name, default `BPFile`
- `--prepend-var-names <str>`: prepends `<str>` to every ADIOS variable name
- `--fixed-dt <float>`: force constant timestep
- `--output-every-steps N`: dump every `N` steps, `0` disables
- `--output-every-time dt`: dump every `dt` in simulation time, `0` disables
- `--save-initial` / `--no-save-initial`
- `--glm-ch`, `--glm-damping` for divergence-cleaning behavior
- `--rho-floor`, `--p-floor` for positivity

## Ensemble Runner

The optional JSON-driven ensemble runner can launch several cases:

```bash
python3 scripts/run_ensembles.py \
  --binary <adios2-build-dir>/bin/mhd_orszag_tang \
  --config ensembles/simple.json
```

Dry run:

```bash
python3 scripts/run_ensembles.py \
  --binary <adios2-build-dir>/bin/mhd_orszag_tang \
  --config ensembles/simple.json \
  --dry-run
```

Available configs:

- `ensembles/simple.json`
- `ensembles/resolution_sweep.json`

The ensemble configs write run output under `runs/` relative to this example
directory unless `--output-dir` is provided.

## Optional Post-Processing

The `scripts/adios_stats.py` and `scripts/adios_derived_variables.py` helpers
can compute simple statistics and derived 2D fields from generated ADIOS2
output in environments that provide NumPy and the ADIOS2 Python Stream API.
