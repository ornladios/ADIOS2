#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

"""Run ensembles of Orszag-Tang simulations with varying solver/parameters."""

from __future__ import annotations

import argparse
import json
import shlex
import subprocess
import sys
from pathlib import Path
from typing import Any


def to_cli_args(options: dict[str, Any]) -> list[str]:
    args: list[str] = []
    for key, value in options.items():
        if value is None:
            continue

        opt = f"--{key.replace('_', '-')}"

        if key == "save_initial":
            args.append("--save-initial" if bool(value) else "--no-save-initial")
            continue

        if isinstance(value, bool):
            if value:
                args.append(opt)
            continue

        args.extend([opt, str(value)])
    return args


def main() -> int:
    parser = argparse.ArgumentParser(description="Run an Orszag-Tang ensemble")
    parser.add_argument("--config", required=True, help="Path to JSON ensemble config")
    parser.add_argument(
        "--binary",
        required=True,
        help="Path to the mhd_orszag_tang executable",
    )
    parser.add_argument(
        "--prepend_var_names",
        "--prepend-var-names",
        dest="prepend_var_names",
        default=None,
        help="Prefix to prepend to all ADIOS variable names for every run",
    )
    parser.add_argument(
        "--fixed-dt",
        "--fixed_dt",
        dest="fixed_dt",
        type=float,
        default=None,
        help="Override all runs with a fixed simulation timestep (disables CFL-based dt)",
    )
    parser.add_argument(
        "--output-dir",
        default=None,
        help="Override the ensemble output root directory",
    )
    parser.add_argument("--dry-run", action="store_true", help="Print commands without running")
    parser.add_argument("--keep-going", action="store_true", help="Continue after failed members")
    parser.add_argument("--only", nargs="*", default=None, help="Run only selected case names")
    parser.add_argument(
        "--ranks",
        type=int,
        default=None,
        help="Override MPI ranks for all runs (default: use per-run or config default)",
    )
    args = parser.parse_args()
    if args.ranks is not None and args.ranks < 1:
        parser.error("--ranks must be >= 1")
    if args.fixed_dt is not None and args.fixed_dt <= 0.0:
        parser.error("--fixed-dt must be > 0")

    config_path = Path(args.config)
    with config_path.open("r", encoding="utf-8") as f:
        cfg = json.load(f)

    base_dir = config_path.resolve().parent

    binary_cfg = Path(args.binary)
    binary = (binary_cfg if binary_cfg.is_absolute() else binary_cfg.resolve()).resolve()
    mpi_launcher = str(cfg.get("mpi_launcher", "mpirun -n {ranks}"))
    output_cfg = (
        Path(args.output_dir)
        if args.output_dir is not None
        else Path(cfg.get("output_dir", "runs"))
    )
    ensemble_output_root = (
        output_cfg if output_cfg.is_absolute() else (base_dir / output_cfg)
    ).resolve()
    common = dict(cfg.get("common_args", {}))
    runs = list(cfg.get("runs", []))

    if not runs:
        print("No runs in config", file=sys.stderr)
        return 1

    ensemble_output_root.mkdir(parents=True, exist_ok=True)

    selected = set(args.only) if args.only else None

    failures = 0
    executed = 0

    for run in runs:
        name = run.get("name")
        if not name:
            print("Skipping entry without 'name'", file=sys.stderr)
            continue

        if selected and name not in selected:
            continue

        ranks = (
            args.ranks
            if args.ranks is not None
            else int(run.get("ranks", cfg.get("default_ranks", 1)))
        )
        run_has_output_file = "output" in run
        run_has_output_dir = "output_dir" in run
        if run_has_output_file and run_has_output_dir:
            print(f"Run '{name}' has both 'output' and 'output_dir'; pick one.", file=sys.stderr)
            return 1

        run_output = None
        run_output_dir = None
        if run_has_output_file:
            run_output_cfg = Path(str(run["output"]))
            run_output = (
                run_output_cfg if run_output_cfg.is_absolute() else (base_dir / run_output_cfg)
            ).resolve()
        else:
            run_output_dir_cfg = Path(str(run.get("output_dir", ensemble_output_root / name)))
            run_output_dir = (
                run_output_dir_cfg
                if run_output_dir_cfg.is_absolute()
                else (base_dir / run_output_dir_cfg)
            ).resolve()

        per_run = {
            k: v
            for k, v in run.items()
            if k not in {"name", "ranks", "output", "output_dir", "mpi_launcher"}
        }
        merged = dict(common)
        merged.update(per_run)
        if args.prepend_var_names is not None:
            merged["prepend_var_names"] = args.prepend_var_names
        if args.fixed_dt is not None:
            merged["fixed_dt"] = args.fixed_dt
        if run_output is not None:
            merged["output"] = str(run_output)
        else:
            assert run_output_dir is not None
            merged["output_dir"] = str(run_output_dir)

        base_cmd = [str(binary)] + to_cli_args(merged)

        if ranks > 1:
            launcher_tmpl = str(run.get("mpi_launcher", mpi_launcher))
            launch_prefix = shlex.split(launcher_tmpl.format(ranks=ranks))
            run_cmd = launch_prefix + base_cmd
        else:
            run_cmd = list(base_cmd)

        executed += 1
        printable = " ".join(shlex.quote(part) for part in run_cmd)
        print(f"[{name}] {printable}")

        if args.dry_run:
            continue

        run_result = subprocess.run(run_cmd)
        if run_result.returncode != 0:
            failures += 1
            print(f"Run '{name}' failed with code {run_result.returncode}", file=sys.stderr)
            if not args.keep_going:
                return run_result.returncode
            continue

    if executed == 0:
        print("No runs selected", file=sys.stderr)
        return 1

    if failures:
        print(f"Finished with {failures} failed run(s)", file=sys.stderr)
        return 1

    print("Ensemble completed successfully")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
