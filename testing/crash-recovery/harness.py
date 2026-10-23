#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0
"""Crash-recovery harness for BP5 and BP4 (--engine, default BP5).

Three modes:
  sweep:  write a good file once, then truncate each constituent file at many
          offsets and require readers to recover or fail cleanly -- never crash.
  kill:   run the writer, SIGKILL it at a random moment, verify that at least
          the steps whose EndStep completed are recovered bit-exact.
  mutate: BP4 only. Overwrite the md.idx fields that the reader uses verbatim
          as read positions into md.0. A corrupt index must raise, not crash
          and not quietly report an empty dataset.

Outcome classes per reader run:
  OK(n)        recovered n steps, content verified
  CLEAN_ERROR  ADIOS threw a catchable exception (acceptable)
  MISMATCH     reader returned wrong data with no error (silent corruption)
  SILENT       reader reported no data and no error for a corrupt index
  SIGNAL       reader died on a signal (segfault etc. -- the 5162 class)
  TIMEOUT      reader hung

bpls is taken from $CR_BPLS when that is set.

Known gap, outside what mutate covers: truncating md.idx or data.0 of a BP4
dataset leaves the readers waiting for a writer that never comes, so
"sweep --engine BP4" reports TIMEOUTs. That is BP4's open and read polling,
not the metadata index bounds checks.
"""

import argparse
import os
import random
import shutil
import signal
import struct
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
BPLS = os.environ.get("CR_BPLS", os.path.expanduser("~/prog/ADIOS2/build/bin/bpls"))
WRITER = os.path.join(HERE, "crashwriter")
READER = os.path.join(HERE, "crashreader")
TIMEOUT_S = int(os.environ.get("HARNESS_TIMEOUT", "2"))


def classify(cmd, cwd=None):
    """Run cmd, return (class, detail)."""
    try:
        # a crashing reader can emit raw heap bytes, which are not UTF-8
        p = subprocess.run(
            cmd, capture_output=True, text=True, errors="replace", timeout=TIMEOUT_S, cwd=cwd
        )
    except subprocess.TimeoutExpired:
        return ("TIMEOUT", "")
    if p.returncode < 0:
        return ("SIGNAL", signal.Signals(-p.returncode).name)
    out = p.stdout.strip().splitlines()
    last = out[-1] if out else ""
    if p.returncode == 0:
        if last.startswith("RECOVERED"):
            return ("OK", last)
        return ("OK", last[:80])
    if p.returncode == 2:
        return ("MISMATCH", last[:120])
    if p.returncode == 3:
        return ("CLEAN_ERROR", last[:120])
    # bpls returns 1 with an error message on stderr for clean failures
    err = p.stderr.strip().splitlines()
    return ("CLEAN_ERROR", (err[-1] if err else last)[:120])


def readers_for(bpdir, engine):
    os.environ.setdefault("CR_EOF_WAIT", "1")
    return [
        ("bpls", [BPLS, "-la", bpdir]),
        ("ra", [READER, bpdir, "ra", engine]),
        ("stream", [READER, bpdir, "stream", engine]),
    ]


def sweep(args):
    src = os.path.join(HERE, "sweep_master.bp")
    shutil.rmtree(src, ignore_errors=True)
    subprocess.run(
        [WRITER, src, str(args.steps), str(args.elems), "0", args.engine],
        check=True,
        capture_output=True,
    )
    work = os.path.join(HERE, "sweep_work.bp")
    bad = []
    total = 0
    files = sorted(f for f in os.listdir(src) if f != "profiling.json")
    for fn in files:
        size = os.path.getsize(os.path.join(src, fn))
        # sample truncation points: all of small files, stride through big ones
        if size <= args.dense_limit:
            points = range(0, size)
        else:
            stride = max(1, size // args.samples)
            points = sorted(set(list(range(0, size, stride)) + [size - 1, size - 2]))
        streak = {}  # rname -> (class, count); skip a reader after 3 identical fails
        for cut in points:
            shutil.rmtree(work, ignore_errors=True)
            shutil.copytree(src, work)
            with open(os.path.join(work, fn), "r+b") as f:
                f.truncate(cut)
            for rname, cmd in readers_for(work, args.engine):
                pc, n = streak.get(rname, (None, 0))
                if n >= 3:
                    continue  # established failure class for this file/reader
                total += 1
                cls, detail = classify(cmd)
                if cls in ("SIGNAL", "TIMEOUT", "MISMATCH"):
                    bad.append((fn, cut, size, rname, cls, detail))
                    print(f"BAD {fn} cut={cut}/{size} {rname}: {cls} {detail}")
                    streak[rname] = (cls, n + 1) if pc == cls else (cls, 1)
                else:
                    streak[rname] = (None, 0)
        for rname, (pc, n) in streak.items():
            if n >= 3:
                print(f"SKIPPED rest of {fn} for {rname}: consistent {pc}")
    print(f"\nsweep done: {total} reader runs, {len(bad)} bad")
    summarize(bad)
    return 1 if bad else 0


def kill_loop(args):
    rng = random.Random(args.seed)
    bad = []
    for it in range(args.iters):
        bpdir = os.path.join(HERE, "kill_work.bp")
        shutil.rmtree(bpdir, ignore_errors=True)
        proc = subprocess.Popen(
            [WRITER, bpdir, str(args.steps), str(args.elems), str(args.sleep_ms), args.engine],
            stdout=subprocess.PIPE,
            text=True,
        )
        delay = rng.uniform(0, args.steps * (args.sleep_ms / 1000.0))
        time.sleep(delay)
        proc.kill()
        out, _ = proc.communicate()
        endsteps = sum(1 for line in out.splitlines() if line.startswith("ENDSTEP"))
        if "CLOSED" in out:
            continue  # writer finished before the kill landed; not interesting
        for rname, cmd in readers_for(bpdir, args.engine):
            cls, detail = classify(cmd)
            fail = cls in ("SIGNAL", "TIMEOUT", "MISMATCH")
            if cls == "OK" and detail.startswith("RECOVERED"):
                n = int(detail.split()[1])
                if n < endsteps:
                    fail, cls, detail = True, "LOST_STEPS", f"recovered {n} < completed {endsteps}"
            if fail:
                bad.append((f"iter{it}", f"{delay:.3f}s", endsteps, rname, cls, detail))
                print(
                    f"BAD iter={it} delay={delay:.3f}s endsteps={endsteps} {rname}: {cls} {detail}"
                )
        if not any(b[0] == f"iter{it}" for b in bad):
            print(f"ok  iter={it} delay={delay:.3f}s endsteps={endsteps}")
    print(f"\nkill loop done: {args.iters} iters, {len(bad)} bad (seed={args.seed})")
    summarize(bad)
    return 1 if bad else 0


def mutate(args):
    """Corrupt the BP4 metadata index, not the metadata it points into.

    md.idx is a 64-byte header followed by one 64-byte record per step. Three
    fields of each record are byte offsets into md.0 that the reader turns
    straight into read positions, so each one is a place where a stale or
    truncated index can send the parser past the end of the metadata buffer.
    Only md.idx is touched here: md.0 and the data files stay byte-identical,
    so every other consistency check in the reader still passes.
    """
    header, record = 64, 64
    fields = [("pgIndexStart", 16), ("variablesIndexStart", 24), ("attributesIndexStart", 32)]

    src = os.path.join(HERE, "mutate_master.bp")
    shutil.rmtree(src, ignore_errors=True)
    subprocess.run(
        [WRITER, src, str(args.steps), str(args.elems), "0", args.engine],
        check=True,
        capture_output=True,
    )
    idx = os.path.join(src, "md.idx")
    md0 = os.path.getsize(os.path.join(src, "md.0"))
    records = (os.path.getsize(idx) - header) // record
    # one past the end of the metadata buffer, then increasingly wild pointers
    values = [md0, 1 << 16, 1 << 30, 1 << 40, 1 << 63, (1 << 64) - 1]

    work = os.path.join(HERE, "mutate_work.bp")
    bad = []
    total = 0
    for rec in range(records):
        for fname, foff in fields:
            at = header + rec * record + foff
            for val in values:
                shutil.rmtree(work, ignore_errors=True)
                shutil.copytree(src, work)
                with open(os.path.join(work, "md.idx"), "r+b") as f:
                    f.seek(at)
                    f.write(struct.pack("<Q", val))
                for rname, cmd in readers_for(work, args.engine):
                    total += 1
                    cls, detail = classify(cmd)
                    if cls == "OK" and (not detail or detail.startswith("RECOVERED 0")):
                        # the index is unusable, so reporting nothing and
                        # exiting 0 hides the corruption from the application
                        cls, detail = "SILENT", detail or "exit 0, nothing reported"
                    if cls in ("SIGNAL", "TIMEOUT", "MISMATCH", "SILENT"):
                        where = f"step{rec}.{fname}"
                        bad.append((where, val, md0, rname, cls, detail))
                        print(f"BAD {where}={val} (md.0={md0}) {rname}: {cls} {detail}")
    print(f"\nmutate done: {total} reader runs, {len(bad)} bad")
    summarize(bad)
    return 1 if bad else 0


def summarize(bad):
    from collections import Counter

    c = Counter((b[0] if not b[0].startswith("iter") else "kill", b[3], b[4]) for b in bad)
    for k, v in sorted(c.items()):
        print(f"  {k[0]:8s} {k[1]:7s} {k[2]:12s} x{v}")


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="mode", required=True)
    sp = sub.add_parser("sweep")
    sp.add_argument("--steps", type=int, default=5)
    sp.add_argument("--elems", type=int, default=1000)
    sp.add_argument("--samples", type=int, default=64, help="truncation points per big file")
    sp.add_argument(
        "--dense-limit",
        type=int,
        default=2048,
        help="files at or below this size get every truncation point",
    )
    kp = sub.add_parser("kill")
    kp.add_argument("--iters", type=int, default=20)
    kp.add_argument("--steps", type=int, default=50)
    kp.add_argument("--elems", type=int, default=100000)
    kp.add_argument("--sleep-ms", type=int, default=20)
    kp.add_argument("--seed", type=int, default=1)
    mp = sub.add_parser("mutate")
    mp.add_argument("--steps", type=int, default=3)
    mp.add_argument("--elems", type=int, default=100)
    for p in (sp, kp):
        p.add_argument("--engine", default="BP5", choices=["BP5", "BP4"])
    # BP5 writes an md.idx too, but with a different record layout, so these
    # field offsets would corrupt arbitrary bytes of it instead
    mp.add_argument("--engine", default="BP4", choices=["BP4"], help="BP4 index layout only")
    args = ap.parse_args()
    sys.exit({"sweep": sweep, "kill": kill_loop, "mutate": mutate}[args.mode](args))
