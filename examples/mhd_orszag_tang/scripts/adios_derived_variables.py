#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

"""Compute derived 2D variables from Orszag-Tang ADIOS2 output files."""

from __future__ import annotations

import argparse
import shutil
from pathlib import Path

import numpy as np

try:
    from adios2.stream import Stream
except ImportError as exc:
    raise SystemExit("This script requires the Python package 'adios2'.") from exc


DEFAULT_OUTPUT_NAME = "analysis.bp"
DERIVED_DEPENDENCY_NAMES = {"rho", "pressure", "bx", "by"}


def getOutputFilename(inputPath: Path, outputName: str = DEFAULT_OUTPUT_NAME) -> Path:
    """Return the default sibling analysis BP path for an input dataset."""
    return inputPath.with_name(outputName)


def resetOutputPath(path: Path) -> None:
    """Remove an existing output file/directory/symlink."""
    if path.is_symlink() or path.is_file():
        path.unlink()
    elif path.is_dir():
        shutil.rmtree(path)


def to2d(field: np.ndarray) -> np.ndarray:
    """Squeeze an ADIOS array to a 2D frame."""
    data = np.asarray(field).squeeze()
    while data.ndim > 2:
        data = data[0]
    if data.ndim != 2:
        raise ValueError(f"Expected 2D data after squeeze, got ndim={data.ndim}")
    return data


def resolveVarName(available: set[str], logicalName: str) -> str | None:
    """Resolve exact or prefix-style variable names such as hll_rho -> rho."""
    if logicalName in available:
        return logicalName

    candidates = [name for name in available if name.endswith(logicalName)]
    if not candidates:
        return None

    sep_candidates = [
        name
        for name in candidates
        if len(name) > len(logicalName) and name[-len(logicalName) - 1] == "_"
    ]
    if sep_candidates:
        candidates = sep_candidates

    return min(candidates, key=len)


def resolveVarNames(available: set[str], logicalNames: set[str]) -> dict[str, str]:
    """Resolve a set of logical variable names against available ADIOS names."""
    resolved: dict[str, str] = {}
    for logicalName in sorted(logicalNames):
        physicalName = resolveVarName(available, logicalName)
        if physicalName is not None:
            resolved[logicalName] = physicalName
    return resolved


def gradient2d(field: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
    """Return dy, dx gradients, preserving the old index-space behavior."""
    return np.gradient(field)


def computeDerivedFields(stepFields: dict[str, np.ndarray]) -> dict[str, np.ndarray]:
    """Compute available derived fields from one timestep."""
    derived: dict[str, np.ndarray] = {}

    rho = stepFields.get("rho")
    if rho is not None:
        d_rho_dy, d_rho_dx = gradient2d(rho)
        derived["grad_rho_abs"] = np.sqrt(d_rho_dx * d_rho_dx + d_rho_dy * d_rho_dy)

    pressure = stepFields.get("pressure")
    if pressure is not None:
        d_p_dy, d_p_dx = gradient2d(pressure)
        derived["grad_pressure_abs"] = np.sqrt(d_p_dx * d_p_dx + d_p_dy * d_p_dy)

    bx = stepFields.get("bx")
    by = stepFields.get("by")
    if bx is not None and by is not None:
        _d_bx_dy, d_bx_dx = gradient2d(bx)
        d_by_dy, _d_by_dx = gradient2d(by)
        derived["div_b"] = d_bx_dx + d_by_dy

    return derived


def processFile(inputPath: Path, outputPath: Path) -> tuple[int, list[str]]:
    """Read one ADIOS dataset and write derived variables to outputPath."""
    if outputPath.exists():
        resetOutputPath(outputPath)

    written_steps = 0
    written_vars: set[str] = set()
    skipped_steps = 0

    with Stream(str(inputPath), "r") as source:
        with Stream(str(outputPath), "w") as sink:
            for step_index, _ in enumerate(source.steps()):
                available = set((source.available_variables() or {}).keys())
                dep_var_names = resolveVarNames(available, DERIVED_DEPENDENCY_NAMES)
                step_fields: dict[str, np.ndarray] = {}

                for dep_name in sorted(DERIVED_DEPENDENCY_NAMES):
                    physical_name = dep_var_names.get(dep_name)
                    if physical_name is None:
                        continue
                    try:
                        arr = np.asarray(source.read(physical_name)).squeeze()
                    except Exception as exc:  # noqa: BLE001
                        print(
                            f"[step {step_index}] skipping dependency '{physical_name}': "
                            f"read failed ({type(exc).__name__}: {exc})"
                        )
                        continue
                    if arr.ndim >= 2:
                        try:
                            step_fields[dep_name] = to2d(arr)
                        except ValueError as exc:
                            print(
                                f"[step {step_index}] skipping dependency '{physical_name}': {exc}"
                            )

                derived = computeDerivedFields(step_fields)
                if not derived:
                    skipped_steps += 1
                    print(f"[step {step_index}] no derived fields available")
                    continue

                sink.begin_step()
                for var_name, frame in sorted(derived.items()):
                    arr2d = np.asarray(frame, dtype=np.float64)
                    ny, nx = arr2d.shape
                    sink.write(var_name, arr2d, [ny, nx], [0, 0], [ny, nx])
                    written_vars.add(var_name)
                sink.end_step()

                written_steps += 1
                print(
                    f"[step {step_index}] written={len(derived)} vars={', '.join(sorted(derived))}"
                )

    if not written_vars and outputPath.exists():
        resetOutputPath(outputPath)

    if skipped_steps:
        print(f"Skipped steps without derived fields: {skipped_steps}")

    return written_steps, sorted(written_vars)


def _format_vars(items: list[str], max_items: int = 10) -> str:
    if len(items) <= max_items:
        return ", ".join(items)
    return f"{', '.join(items[:max_items])}, ... (+{len(items) - max_items} more)"


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


def _parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Compute derived Orszag-Tang variables from ADIOS2 files "
            "and write results to sibling analysis BP files."
        )
    )
    parser.add_argument(
        "--input",
        nargs="+",
        required=True,
        help=(
            "Single-file mode: one input file path. "
            "Recursive mode (--input_dir): one or more filenames/patterns to match."
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
    parser.add_argument(
        "--outputName",
        default=DEFAULT_OUTPUT_NAME,
        help=(
            "Output BP name for recursive mode and single-file defaults. "
            f"Default: {DEFAULT_OUTPUT_NAME}"
        ),
    )
    return parser.parse_args()


def _process_single_input(inputPath: Path, outputPath: Path) -> None:
    print(f"Input : {inputPath}")
    print(f"Output: {outputPath}")

    written_steps, written_vars = processFile(inputPath, outputPath)

    vars_text = _format_vars(written_vars)
    print(f"Completed: steps={written_steps}, output_variables={len(written_vars)} ({vars_text})")


def _process_recursive_inputs(input_dir: Path, input_names: list[str], output_name: str) -> None:
    matches = _discover_recursive_inputs(input_dir, input_names)
    if not matches:
        raise SystemExit(f"No matching input files found under: {input_dir}")

    print(f"Input dir: {input_dir}")
    print(f"Requested names: {', '.join(input_names)}")
    print(f"Output name: {output_name}")
    print(f"Matched files: {len(matches)}")

    processed = 0
    failed: list[str] = []
    total_steps = 0
    all_written_vars: set[str] = set()

    for inputPath in matches:
        outputPath = getOutputFilename(inputPath, output_name)
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

        _process_recursive_inputs(input_dir, [str(x) for x in args.input], str(args.outputName))
        return

    if len(args.input) != 1:
        raise SystemExit("Single-file mode requires exactly one --input path.")

    inputPath = Path(args.input[0]).expanduser().resolve()
    if not inputPath.exists():
        raise SystemExit(f"Input file not found: {inputPath}")

    outputPath = (
        Path(args.output).expanduser().resolve()
        if args.output
        else getOutputFilename(inputPath, str(args.outputName))
    )
    _process_single_input(inputPath, outputPath)


if __name__ == "__main__":
    main()
