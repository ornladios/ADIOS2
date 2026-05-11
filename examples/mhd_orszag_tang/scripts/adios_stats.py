#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

"""Compute per-timestep statistics for numeric variables in ADIOS2 files."""

from __future__ import annotations

import argparse
import shutil
from pathlib import Path

import numpy as np

try:
    from adios2.stream import Stream
except ImportError as exc:
    raise SystemExit("This script requires the Python package 'adios2'.") from exc


STAT_NAMES = ("min", "max", "mean", "median", "std", "var")


def getOutputFilename(inputPath: Path) -> Path:
    """Return default output path by appending '_stats' before extension."""
    if inputPath.suffix:
        return inputPath.with_name(f"{inputPath.stem}_stats{inputPath.suffix}")
    return inputPath.with_name(f"{inputPath.name}_stats")


def resetOutputPath(path: Path) -> None:
    """Remove an existing output file/directory/symlink."""
    if path.is_symlink() or path.is_file():
        path.unlink()
    elif path.is_dir():
        shutil.rmtree(path)


def isNumericNdarray(arr: np.ndarray) -> bool:
    """Return True only for integer/float ndarray dtypes."""
    dtype = np.asarray(arr).dtype
    return bool(np.issubdtype(dtype, np.integer) or np.issubdtype(dtype, np.floating))


def computeStats(arr: np.ndarray) -> dict[str, float]:
    """Compute min/max/mean/median/std/var over all elements using float64."""
    flat64 = np.asarray(arr, dtype=np.float64).ravel()
    return {
        "min": float(np.min(flat64)),
        "max": float(np.max(flat64)),
        "mean": float(np.mean(flat64, dtype=np.float64)),
        "median": float(np.median(flat64)),
        "std": float(np.std(flat64, dtype=np.float64)),
        "var": float(np.var(flat64, dtype=np.float64)),
    }


def _format_skip_list(items: list[str], max_items: int = 8) -> str:
    if not items:
        return "none"
    if len(items) <= max_items:
        return ", ".join(items)
    shown = ", ".join(items[:max_items])
    return f"{shown}, ... (+{len(items) - max_items} more)"


def _discover_recursive_inputs(input_dir: Path, input_names: list[str]) -> list[Path]:
    """Find files/dirs under input_dir that match any provided basename/pattern."""
    found: dict[str, Path] = {}

    for raw_name in input_names:
        name = str(raw_name or "").strip()
        if not name:
            continue

        matches = sorted(input_dir.rglob(name))
        if not matches:
            print(f"warning: no matches for '{name}' under {input_dir}")
            continue

        for match in matches:
            if not (match.is_file() or match.is_dir()):
                continue
            resolved = match.resolve()
            found[str(resolved)] = resolved

    return sorted(found.values())


def processFile(inputPath: Path, outputPath: Path) -> tuple[int, list[str]]:
    """Read per-step variables, then write each stat variable once as shape (1, N)."""
    if outputPath.exists():
        resetOutputPath(outputPath)

    written_steps = 0
    written_vars: set[str] = set()
    # out_name -> length-N list of per-step statistic values (NaN when missing)
    time_series: dict[str, list[float]] = {}

    with Stream(str(inputPath), "r") as source:
        for step_index, _ in enumerate(source.steps()):
            available = source.available_variables() or {}
            variable_names = sorted(available.keys())

            skipped_non_numeric: list[str] = []
            skipped_read_failed: list[str] = []
            skipped_empty: list[str] = []

            step_stats: dict[str, float] = {}

            for varName in variable_names:
                try:
                    arr = np.asarray(source.read(varName)).squeeze()
                except Exception as exc:  # noqa: BLE001
                    error_name = type(exc).__name__
                    print(
                        f"[step {step_index}] skipping '{varName}': "
                        f"read failed ({error_name}: {exc})"
                    )
                    skipped_read_failed.append(varName)
                    continue

                if arr.size == 0:
                    print(f"[step {step_index}] skipping '{varName}': empty array")
                    skipped_empty.append(varName)
                    continue

                if not isNumericNdarray(arr):
                    print(
                        f"[step {step_index}] skipping '{varName}': non-numeric dtype={arr.dtype}"
                    )
                    skipped_non_numeric.append(varName)
                    continue

                stats = computeStats(arr)
                for stat_name in STAT_NAMES:
                    outName = f"{varName}_{stat_name}"
                    step_stats[outName] = stats[stat_name]
                    written_vars.add(outName)

            # Ensure new variables are backfilled for earlier steps.
            for outName in step_stats:
                if outName not in time_series:
                    time_series[outName] = [float("nan")] * written_steps

            # Append this timestep for every known output variable.
            for outName, series in time_series.items():
                series.append(float(step_stats.get(outName, float("nan"))))

            skipped_total = len(skipped_non_numeric) + len(skipped_read_failed) + len(skipped_empty)
            print(
                f"[step {step_index}] vars={len(variable_names)} "
                f"written={len(step_stats)} skipped={skipped_total}"
            )
            if skipped_non_numeric:
                print(
                    f"  non-numeric({len(skipped_non_numeric)}): "
                    f"{_format_skip_list(skipped_non_numeric)}"
                )
            if skipped_read_failed:
                print(
                    f"  read-failed({len(skipped_read_failed)}): "
                    f"{_format_skip_list(skipped_read_failed)}"
                )
            if skipped_empty:
                print(f"  empty({len(skipped_empty)}): {_format_skip_list(skipped_empty)}")

            written_steps += 1

    # Write one output step: each variable is a 1xN array across all timesteps.
    with Stream(str(outputPath), "w") as sink:
        sink.begin_step()
        for outName in sorted(time_series.keys()):
            values = np.asarray(time_series[outName], dtype=np.float64).reshape(1, written_steps)
            sink.write(outName, values, [1, written_steps], [0, 0], [1, written_steps])
        sink.end_step()

    return written_steps, sorted(written_vars)


def _parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Compute per-timestep min/max/mean/median/std/var for each numeric variable "
            "in ADIOS2 files and write results to sibling *_stats files."
        )
    )
    parser.add_argument(
        "--input",
        nargs="+",
        required=True,
        help=(
            "Single-file mode: one input file path. "
            "Recursive mode (--input_dir): one or more filenames/patterns to match "
            "(e.g., output.bp analysis.bp)."
        ),
    )
    parser.add_argument(
        "--input_dir",
        default="",
        help="Optional root directory to search recursively for --input filenames.",
    )
    parser.add_argument(
        "--output",
        required=False,
        default="",
        help="Optional output file path (valid only in single-file mode).",
    )
    return parser.parse_args()


def _process_single_input(inputPath: Path, outputPath: Path) -> None:
    print(f"Input : {inputPath}")
    print(f"Output: {outputPath}")

    written_steps, written_vars = processFile(inputPath, outputPath)

    print(
        f"Completed: steps={written_steps}, output_variables={len(written_vars)} "
        f"({', '.join(written_vars[:10])}{'...' if len(written_vars) > 10 else ''})"
    )


def _process_recursive_inputs(input_dir: Path, input_names: list[str]) -> None:
    matches = _discover_recursive_inputs(input_dir, input_names)
    if not matches:
        raise SystemExit(f"No matching input files found under: {input_dir}")

    print(f"Input dir: {input_dir}")
    print(f"Requested names: {', '.join(input_names)}")
    print(f"Matched files: {len(matches)}")

    processed = 0
    failed: list[str] = []
    total_steps = 0
    all_written_vars: set[str] = set()

    for inputPath in matches:
        outputPath = getOutputFilename(inputPath)
        print("-" * 80)
        print(f"Input : {inputPath}")
        print(f"Output: {outputPath}")
        try:
            written_steps, written_vars = processFile(inputPath, outputPath)
            total_steps += written_steps
            all_written_vars.update(written_vars)
            processed += 1
            print(f"Completed file: steps={written_steps}, output_variables={len(written_vars)}")
        except Exception as exc:  # noqa: BLE001
            failed.append(str(inputPath))
            print(f"ERROR: failed processing {inputPath}: {type(exc).__name__}: {exc}")

    print("=" * 80)
    print(
        f"Summary: processed={processed}, failed={len(failed)}, "
        f"total_steps={total_steps}, unique_output_variables={len(all_written_vars)}"
    )
    if failed:
        print("Failed inputs:")
        for path in failed:
            print(f"  {path}")


def main() -> None:
    args = _parse_args()

    input_dir_raw = str(args.input_dir or "").strip()
    if input_dir_raw:
        if args.output:
            raise SystemExit(
                "--output is only supported in single-file mode (without --input_dir)."
            )

        input_dir = Path(input_dir_raw).expanduser().resolve()
        if not input_dir.is_dir():
            raise SystemExit(f"input_dir is not a directory: {input_dir}")

        _process_recursive_inputs(input_dir, [str(x) for x in args.input])
        return

    if len(args.input) != 1:
        raise SystemExit("Single-file mode requires exactly one --input path.")

    inputPath = Path(args.input[0]).expanduser().resolve()
    if not inputPath.exists():
        raise SystemExit(f"Input file not found: {inputPath}")

    outputPath = (
        Path(args.output).expanduser().resolve() if args.output else getOutputFilename(inputPath)
    )
    _process_single_input(inputPath, outputPath)


if __name__ == "__main__":
    main()
