# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

# Sourced by qualify.sh, which reads these settings and sets site, cmd, ROOT and SELF.
# shellcheck shell=bash disable=SC1091,SC2034,SC2154

# The DAOS engine tests (ctest -L DAOS) on one Aurora node.  aurora.sh runs the data planes and
# GPU tests.  The DAOS tests launch with --no-vni, which RDMA cannot use, so they get their own
# build.
ACCOUNT="${ACCOUNT:-Optimizing_ADIOS_IO}"
SITE_ROOT="$HOME/adios2-qualification"

CMAKE_ARGS=(-DCMAKE_C_COMPILER=icx -DCMAKE_CXX_COMPILER=icpx -DCMAKE_Fortran_COMPILER=ifx
    -DADIOS2_USE_DAOS=ON -DADIOS2_RUN_DAOS_TESTS=ON -DADIOS2_USE_Kokkos=OFF
    "-DMPIEXEC_EXTRA_FLAGS=--no-vni -genvall --cpu-bind numa")
REQUIRE_CACHE=(ADIOS2_USE_DAOS ADIOS2_RUN_DAOS_TESTS)

DATAPLANES=""
CTEST_J=4
SITE_PHASES=daos

site_env()
{
    command -v module >/dev/null || source /etc/profile
    module use /soft/modulefiles
    module load cmake
    [ "$cmd" = run ] || return 0
    # The daos/base environment has hung BP5 on Lustre, so it is loaded only for the run, and
    # the build and its test output stay on home.
    module unload daos_ops 2>/dev/null
    module load daos/base
    export DAOS_AGENT_DRPC_DIR=/run/daos_agent_oneScratch
    if [ ! -S "$DAOS_AGENT_DRPC_DIR/daos_agent.sock" ]; then
        echo "!! no daos_user agent at $DAOS_AGENT_DRPC_DIR" >&2
        return 1
    fi
    export FI_CXI_OFLOW_BUF_SIZE=8388608 FI_CXI_CQ_FILL_PERCENT=20
}

# Each run gets a new container, destroyed on exit.  Datasets left by an earlier run of the
# same build are removed first.
site_phases()
{
    export DAOS_POOL="${DAOS_POOL:-$ACCOUNT}"
    export DAOS_CONT="qualify-$$"
    if ! daos cont create "$DAOS_POOL" "$DAOS_CONT" >/dev/null; then
        record "daos FAILED cannot create container $DAOS_CONT in pool $DAOS_POOL"
        return 1
    fi
    trap 'daos cont destroy "$DAOS_POOL" "$DAOS_CONT" --force >/dev/null 2>&1' EXIT
    rm -rf "$BUILD"/testing/adios2/engine/bp/daos/* "$BUILD"/testing/adios2/engine/bp/operations/daos/*
    run_ctest daos -L DAOS --output-on-failure
}

site_submit()
{
    qsub -A "$ACCOUNT" -N adios2-qualify-daos -q debug -l select=1 -l walltime=01:00:00 \
        -l filesystems=home:daos_user -l daos=daos_user -j oe -o "$ROOT/" -- "$SELF" "$site" run
}
