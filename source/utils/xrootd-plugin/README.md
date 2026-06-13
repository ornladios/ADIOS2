<!--
SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
SPDX-License-Identifier: Apache-2.0
-->

# ADIOS2 XRootD Server Plugin

Server-side XRootD plugin that serves remote reads of ADIOS2 (BP5) datasets over
the XRootD SSI and HTTP(S) interfaces. It is the data-plane backend for remote
campaign access. Two libraries are built here:

- `adios2_xrootd` — the SSI service (request processing, file pool).
- `adios2_xrootd_http` — the HTTP-to-SSI bridge handler.

This document covers operator-facing configuration. Other operational knobs
(file-pool FD/metadata limits, the admin HTTP interface) are described in the
release notes and will be folded in here over time.

## Limits

A single response (one variable get, or one batch frame) is capped at 2 GiB, a
bound imposed by the XRootD SSI response API. A request whose result would
exceed it is rejected with an error telling the client to request a smaller
selection or fewer variables per batch. Lifting the cap requires a streaming
response path, which is not yet supported.

## Access log

An optional per-request access log records every remote read as one JSON object
per line, for offline analysis of access patterns: which variables, steps, and
regions are read. It is **off by default** and does not affect request handling.
A dedicated writer thread performs all file I/O, so request threads never block
on logging.

### Configuration

All configuration is via environment variables on the XRootD server process:

| Variable | Default | Meaning |
|---|---|---|
| `ADIOS2_XROOTD_ACCESSLOG` | (unset) | Path to the active log file. Logging is **off** unless this is set to a writable path. |
| `ADIOS2_XROOTD_ACCESSLOG_MAXSIZE` | `134217728` (128 MB) | Rotate the active file once it exceeds this many bytes. `0` disables rotation (a single growing file). |
| `ADIOS2_XROOTD_ACCESSLOG_KEEP` | `16` | Number of rotated segments to retain; older ones are deleted. With the defaults the log is capped at roughly 2 GB. |

### Record format

One JSON object per line. The first line of each file is a schema marker,
`{"_schema":1}`. Each request record contains:

| Field | Meaning |
|---|---|
| `ts` | Wall-clock time, epoch seconds (floating point). |
| `file` | Dataset path being read. |
| `var` | Variable name. |
| `step`, `nsteps` | First step and number of steps in the read. |
| `start`, `count` | Selection bounding box; omitted for a whole-variable read. |
| `block` | Block id, when a block selection was requested. |
| `acc` | Requested accuracy error, when a lossy accuracy was requested. |
| `bytes` | Bytes of data returned for this record. |
| `batch` | Number of variables in the batch this record belonged to (`1` for a single get). |

Example:

```json
{"_schema":1}
{"ts":1717286400.123,"file":"sim.bp","var":"temperature","step":42,"nsteps":1,"start":[0,0],"count":[128,256],"bytes":131072,"batch":1}
```

### Rotation, retention, and restart

- **Rotation** is size-based and runs on the writer thread. When the active file
  exceeds `MAXSIZE` it is renamed to `<path>.<UTC-timestamp>` (e.g.
  `access.jsonl.20260603T154122`; a `-N` suffix disambiguates same-second
  rotations) and a fresh active file is opened. Each segment starts with its own
  `{"_schema":1}` line so it can be analyzed in isolation.
- **Retention** keeps the newest `KEEP` rotated segments and deletes the rest.
- **Restart**: on startup the server appends to the existing active file and
  resumes rotation accounting from its current size, so a restart continues the
  log rather than truncating it.

### Notes

- The in-memory handoff queue is bounded. If a burst outruns the writer, records
  are dropped rather than blocking the request, and a `{"dropped":N}` line
  records how many.
- A hard crash loses at most the last ~250 ms of buffered records; a clean
  shutdown flushes everything.
- Rotation is by size only. Time-based (e.g. daily) rotation may be added if
  large-scale production use calls for a fixed time window per segment.
