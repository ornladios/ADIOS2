#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

# Compile the Kotoba v1 ADIOS2/BP identification-field module with CLI 0.7.2.
# The fixture file is 64 bytes; the module special-cases bytes 0-7 and 36-39.
# Owner constraint: magic/header only — no engine, writer, or timestep/query.
# Do not invent a pass: every gate reads the fixture file, a compiler receipt,
# or a runtime value. A local runtime value is not a CI result.

set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
MODULE="${ROOT}/adios2.kotoba"
FIXTURE="${ROOT}/fixtures/bp5-index-header.bin"
EXPECTED_HEX="4144494f532d42502076322e312e3020496e646578205461626c65000000000032313000000502006e0004030201000000000000000000000000000000000000"
EXPECTED_VALUE="110520"

KOTOBA_VERSION="0.7.2"
KOTOBA_TARBALL="kotoba-linux-amd64.tar.gz"
KOTOBA_URL="https://github.com/kotoba-lang/kotoba/releases/download/v${KOTOBA_VERSION}/${KOTOBA_TARBALL}"
KOTOBA_SHA256="95e225461e1b8a21849b251e8c8b654693d2c8a516b258532771651e978e1977"

fail() {
  echo "FAIL: $*" >&2
  exit 1
}

need_cmd() {
  command -v "$1" >/dev/null 2>&1 || fail "required command not found: $1"
}

need_cmd python3
need_cmd curl
need_cmd tar
need_cmd sha256sum
need_cmd grep

[[ -f "${MODULE}" ]] || fail "missing module ${MODULE}"
[[ -f "${FIXTURE}" ]] || fail "missing fixture ${FIXTURE}"

python3 - "${FIXTURE}" "${MODULE}" "${EXPECTED_HEX}" <<'PY'
import pathlib
import re
import sys

fixture = pathlib.Path(sys.argv[1]).read_bytes()
module_text = pathlib.Path(sys.argv[2]).read_text()
expected_hex = sys.argv[3]

if len(fixture) != 64:
    raise SystemExit(f"fixture length {len(fixture)}, expected 64")
if fixture[:8] != b"ADIOS-BP":
    raise SystemExit(f"fixture magic {fixture[:8]!r}, expected b'ADIOS-BP'")
if fixture[16:27] != b"Index Table":
    raise SystemExit(f"fixture file type {fixture[16:27]!r}, expected b'Index Table'")
if fixture[36] != 0:
    raise SystemExit(f"fixture endian byte {fixture[36]}, expected 0 (little)")
if fixture[37] != 5:
    raise SystemExit(f"fixture BP version {fixture[37]}, expected 5")
if fixture[38] != 2:
    raise SystemExit(f"fixture BP minor {fixture[38]}, expected 2")
if fixture[39] != 0:
    raise SystemExit(f"fixture active flag {fixture[39]}, expected 0")

got_hex = fixture.hex()
if got_hex != expected_hex:
    raise SystemExit(f"fixture hex {got_hex}, expected {expected_hex}")

comment = None
for line in module_text.splitlines():
    if line.startswith(";; Vendored fixture bytes (hex): "):
        comment = line.split(": ", 1)[1].strip().lower()
        break
if comment != expected_hex:
    raise SystemExit(f"module fixture comment {comment!r}, expected {expected_hex}")

wanted = {
    "header-len": "64",
    "magic-0": "65",
    "magic-1": "68",
    "magic-2": "73",
    "magic-3": "79",
    "magic-4": "83",
    "magic-5": "45",
    "magic-6": "66",
    "magic-7": "80",
    "endian-flag": "0",
    "bp-version": "5",
    "bp-minor": "2",
    "active-flag": "0",
}
for name, lit in wanted.items():
    if not re.search(rf"\(defn {name} \[\] {lit}\)", module_text):
        raise SystemExit(f"module is missing (defn {name} [] {lit})")

magic = bytes(int(wanted[f"magic-{i}"]) for i in range(8))
if magic != fixture[:8]:
    raise SystemExit(f"module magic literals {magic!r} != fixture {fixture[:8]!r}")
if int(wanted["endian-flag"]) != fixture[36]:
    raise SystemExit("module endian-flag does not match fixture byte 36")
if int(wanted["bp-version"]) != fixture[37]:
    raise SystemExit("module bp-version does not match fixture byte 37")
if int(wanted["bp-minor"]) != fixture[38]:
    raise SystemExit("module bp-minor does not match fixture byte 38")
if int(wanted["active-flag"]) != fixture[39]:
    raise SystemExit("module active-flag does not match fixture byte 39")

print(
    "fixture: 64-byte ADIOS-BP Index Table, LE, BP 5.2, active=0; module literals match"
)
PY

if [[ -n "${KOTOBA_BIN:-}" ]]; then
  KOTOBA="${KOTOBA_BIN}"
  [[ -x "${KOTOBA}" ]] || fail "KOTOBA_BIN is not executable: ${KOTOBA}"
elif [[ -n "${KOTOBA:-}" ]]; then
  [[ -x "${KOTOBA}" ]] || fail "KOTOBA is not executable: ${KOTOBA}"
else
  uname_s="$(uname -s)"
  uname_m="$(uname -m)"
  if [[ "${uname_s}" != "Linux" || "${uname_m}" != "x86_64" ]]; then
    fail "no KOTOBA/KOTOBA_BIN set; automatic install is linux-amd64 only (this host is ${uname_s}/${uname_m})"
  fi
  CACHE="${KOTOBA_CACHE:-${XDG_CACHE_HOME:-${HOME}/.cache}/kotoba-cli/${KOTOBA_VERSION}}"
  mkdir -p "${CACHE}"
  TARBALL="${CACHE}/${KOTOBA_TARBALL}"
  if [[ ! -x "${CACHE}/kotoba" ]]; then
    echo "downloading Kotoba CLI ${KOTOBA_VERSION}"
    curl -fsSL -o "${TARBALL}" "${KOTOBA_URL}"
    echo "${KOTOBA_SHA256}  ${TARBALL}" | sha256sum -c -
    tar -xzf "${TARBALL}" -C "${CACHE}" kotoba
  fi
  KOTOBA="${CACHE}/kotoba"
  [[ -x "${KOTOBA}" ]] || fail "downloaded kotoba binary missing at ${KOTOBA}"
fi

WORKDIR="$(mktemp -d)"
trap 'rm -rf "${WORKDIR}"' EXIT
WASM="${WORKDIR}/adios2.wasm"
COMPILE_JSON="${WORKDIR}/compile.json"
RUN_JSON="${WORKDIR}/run.json"

echo "compile ${MODULE} with ${KOTOBA}"
if ! "${KOTOBA}" compile "${MODULE}" --target wasm -o "${WASM}" --json >"${COMPILE_JSON}"; then
  echo "compile stdout/stderr follows" >&2
  cat "${COMPILE_JSON}" >&2 || true
  fail "kotoba compile exited non-zero"
fi

python3 - "${COMPILE_JSON}" "${WASM}" <<'PY'
import json
import pathlib
import sys

raw = pathlib.Path(sys.argv[1]).read_text()
wasm = pathlib.Path(sys.argv[2]).read_bytes()
try:
    receipt = json.loads(raw)
except json.JSONDecodeError as exc:
    raise SystemExit(f"compile output is not JSON: {exc}: {raw!r}") from exc
if receipt.get("kotoba.cli/ok?") is not True:
    raise SystemExit(f"compile ok? {receipt.get('kotoba.cli/ok?')}: {receipt}")
if receipt.get("kotoba.cli/code") != "emitted":
    raise SystemExit(f"compile code {receipt.get('kotoba.cli/code')}: {receipt}")
data = receipt.get("kotoba.cli/data") or {}
if data.get("value-profile") != "i64-v1":
    raise SystemExit(f"value-profile {data.get('value-profile')!r}, expected i64-v1")
compat = data.get("compatibility") or {}
if compat.get("target") != "wasm32-kotoba-v1":
    raise SystemExit(f"target {compat.get('target')!r}, expected wasm32-kotoba-v1")
if data.get("value-abi") != "direct-v1":
    raise SystemExit(f"value-abi {data.get('value-abi')!r}, expected direct-v1")
features = data.get("wasm-features") or []
if features:
    raise SystemExit(f"wasm-features {features!r}, expected none for i64-v1")
if wasm[:4] != b"\x00asm":
    raise SystemExit("compiled artifact is not a wasm module")
if b"wasm32-kotoba-v1" not in wasm:
    raise SystemExit("compiled artifact is missing wasm32-kotoba-v1 target mark")
print("compile receipt: i64-v1 wasm32-kotoba-v1 direct-v1")
PY

echo "run ${MODULE}"
if ! "${KOTOBA}" run "${MODULE}" --json >"${RUN_JSON}"; then
  echo "run stdout/stderr follows" >&2
  cat "${RUN_JSON}" >&2 || true
  fail "kotoba run exited non-zero"
fi

python3 - "${RUN_JSON}" "${EXPECTED_VALUE}" <<'PY'
import json
import pathlib
import sys

raw = pathlib.Path(sys.argv[1]).read_text()
expected = int(sys.argv[2])
try:
    receipt = json.loads(raw)
except json.JSONDecodeError as exc:
    raise SystemExit(f"run output is not JSON: {exc}: {raw!r}") from exc
if receipt.get("kotoba.cli/ok?") is not True:
    raise SystemExit(f"run ok? {receipt.get('kotoba.cli/ok?')}: {receipt}")
if receipt.get("kotoba.cli/code") != "completed":
    raise SystemExit(f"run code {receipt.get('kotoba.cli/code')}: {receipt}")
data = receipt.get("kotoba.cli/data") or {}
result = data.get("kotoba.runtime/result") or {}
if result.get("kotoba.runtime/ok?") is not True:
    raise SystemExit(f"runtime ok? {result.get('kotoba.runtime/ok?')}: {receipt}")
value = result.get("kotoba.runtime/value")
if value != expected:
    raise SystemExit(f"runtime value {value!r}, expected {expected}")
print(f"run result: {value}")
PY

echo "PASS: Kotoba v1 ADIOS2/BP5 header fields ${EXPECTED_VALUE}"
