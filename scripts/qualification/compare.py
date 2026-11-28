#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

"""Report what got worse between two qualification result directories.

Exits nonzero on a regression: a test that passed in the baseline and did not
pass in the new run, including one that disappeared, or a phase that ran in the
baseline and has no results now although the new run meant to run it.  recheck-
phases rerun failures under evpath and are shown, not counted.
"""

import argparse
import pathlib
import sys
import xml.etree.ElementTree as ET


def load(results):
    phases = {}
    for path in sorted(pathlib.Path(results).glob("*.xml")):
        phases[path.stem] = {
            case.get("name"): case.get("status", "run") for case in ET.parse(path).iter("testcase")
        }
    return phases


def meta(results):
    path = pathlib.Path(results) / "meta.txt"
    return dict(line.split("=", 1) for line in path.read_text().splitlines() if "=" in line)


def main():  # noqa: PLR0912
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("baseline")
    parser.add_argument("new")
    args = parser.parse_args()

    old_phases, new_phases = load(args.baseline), load(args.new)
    old_meta, new_meta = meta(args.baseline), meta(args.new)
    print(f"baseline {old_meta.get('describe')}  new {new_meta.get('describe')}")
    if old_meta.get("site") != new_meta.get("site"):
        print(f"!! comparing different sites: {old_meta.get('site')}, {new_meta.get('site')}")
    if new_meta.get("tree_changes", "0") != "0":
        print("!! the new run had uncommitted changes")

    # Runs before meta.txt listed its phases meant to run all of the baseline's.
    planned = new_meta.get("phases")
    planned = set(planned.split()) if planned is not None else set(old_phases)

    regressions = 0
    for phase in sorted(set(old_phases) | set(new_phases)):
        old, new = old_phases.get(phase), new_phases.get(phase)
        if phase.startswith("recheck-"):
            if new is not None:
                passed = sum(status == "run" for status in new.values())
                print(f"{phase}: {passed}/{len(new)} of these failures pass under evpath")
            continue
        if new is None:
            if phase in planned:
                print(f"{phase}: REGRESSED, no results in the new run, see SUMMARY.txt")
                regressions += 1
            else:
                print(f"{phase}: not run, no longer a phase on this site")
            continue
        passed = sum(status == "run" for status in new.values())
        print(f"{phase}: {passed}/{len(new)} passed")
        if old is None:
            print("  no baseline for this phase")
            old = {}
        for test in sorted(set(old) | set(new)):
            before, after = old.get(test), new.get(test)
            if before == "run" and after != "run":
                print(f"  REGRESSED {test}: {after or 'missing'}")
                regressions += 1
            elif before != "run" and after == "run":
                print(f"  fixed or new {test}")
            elif after is not None and after != "run":
                print(f"  still not passing {test}: {after}")

    print(f"{regressions} regression(s)")
    return 1 if regressions else 0


if __name__ == "__main__":
    sys.exit(main())
