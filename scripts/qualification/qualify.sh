#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

# Builds and runs the tests CI cannot run.  See README.md.
#   qualify.sh <site> build     on a login node
#   qualify.sh <site> submit    submit the run to the scheduler
#   qualify.sh <site> run       inside an allocation

set -o pipefail

site="$1"
cmd="$2"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck disable=SC2034 # used by the site files
SELF="$HERE/qualify.sh"
if [ ! -f "$HERE/sites/$site.sh" ] || [ -z "$cmd" ]; then
    echo "usage: qualify.sh <site> build|submit|run" >&2
    echo "sites: $(for f in "$HERE"/sites/*.sh; do basename "$f" .sh; done | xargs)" >&2
    exit 2
fi

# Exits as soon as a test binary reaches SST open, so a refused dataplane fails
# fast.  A writer and reader pair would wait out the test timeout instead.
PROBE_TEST='^Engine\.SST\.SstWriteFails\.InvalidBeginStep\.MPI$'
SST_TESTS='^Staging\..*\.SST'
NO_DATAPLANE=qualification-no-such-dataplane

SRC="$(git -C "$HERE" rev-parse --show-toplevel)"
SHA="$(git -C "$SRC" rev-parse --short=12 HEAD)"

site_settle() { :; }
# Runs site phases that need their own setup, after the GPU phase.
site_phases() { :; }
# shellcheck disable=SC1090
source "$HERE/sites/$site.sh"
ROOT="${QUAL_ROOT:-$SITE_ROOT}/$site"
BUILD="${QUAL_BUILD:-$ROOT/build-$SHA}"
site_env || exit 1

build_kokkos()
{
    local prefix="$ROOT/kokkos-$KOKKOS_VERSION"
    if [ ! -d "$prefix" ]; then
        mkdir -p "$ROOT/deps" || return 1
        curl -L "https://github.com/kokkos/kokkos/archive/refs/tags/$KOKKOS_VERSION.tar.gz" |
            tar -C "$ROOT/deps" -xzf - || return 1
        cmake -S "$ROOT/deps/kokkos-$KOKKOS_VERSION" -B "$ROOT/deps/kokkos-build-$KOKKOS_VERSION" \
            -DCMAKE_INSTALL_PREFIX="$prefix" -DCMAKE_BUILD_TYPE=Release \
            -DBUILD_SHARED_LIBS=ON -DCMAKE_CXX_STANDARD=17 -DCMAKE_CXX_EXTENSIONS=OFF \
            -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DKokkos_ENABLE_SERIAL=ON \
            "${KOKKOS_ARGS[@]}" &&
            cmake --build "$ROOT/deps/kokkos-build-$KOKKOS_VERSION" -j "${BUILD_J:-16}" &&
            cmake --install "$ROOT/deps/kokkos-build-$KOKKOS_VERSION" || return 1
    fi
    CMAKE_ARGS+=("-DKokkos_ROOT=$prefix")
}

do_build()
{
    if [ -n "$KOKKOS_VERSION" ]; then
        build_kokkos || exit 1
    fi
    cmake -S "$SRC" -B "$BUILD" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=ON \
        -DADIOS2_USE_MPI=ON -DADIOS2_USE_SST=ON "${CMAKE_ARGS[@]}" || exit 1
    # AUTO options degrade silently, so a feature this site exists to test must be present.
    local var
    for var in "${REQUIRE_CACHE[@]}"; do
        if ! grep -q -i -E "^$var:[A-Z]*=(1|ON|TRUE)$" "$BUILD/CMakeCache.txt"; then
            echo "!! $var is not on in this build, so its tests would not mean anything" >&2
            exit 1
        fi
    done
    cmake --build "$BUILD" -j "${BUILD_J:-16}" || exit 1
    echo ">> built $SHA in $BUILD"
}

record()
{
    echo "$1" | tee -a "$RESULTS/SUMMARY.txt"
}

# ctest writes the junit file only when it finishes, so a phase with no xml did not finish.
run_ctest()
{
    local phase="$1"
    shift
    local exclude=()
    if [ -n "$EXCLUDE" ]; then
        exclude=(-E "$EXCLUDE")
    fi
    (cd "$BUILD" && ctest -j "${CTEST_J:-8}" --no-tests=error "${exclude[@]}" \
        --output-junit "$RESULTS/$phase.xml" "$@") >"$RESULTS/$phase.log" 2>&1
    local rc=$?
    record "$phase rc=$rc $(grep -E 'tests passed|tests failed' "$RESULTS/$phase.log" | tail -1)"
}

# CI covers evpath, so it is not a phase here.  Rerunning a data plane's failures under evpath
# on the same nodes tells a data plane problem from a test or machine problem.
recheck()
{
    [ "$1" = evpath ] && return 0
    local failed
    failed="$(sed -n 's/^[[:space:]]*[0-9]* - \(.*\) ([^)]*)$/\1/p' "$RESULTS/sst-$1.log" |
        sed 's/\./\\./g' | paste -sd'|' -)"
    [ -n "$failed" ] || return 0
    site_settle
    SST_REQUIRE_DATAPLANE=evpath run_ctest "recheck-$1" -R "^($failed)\$"
}

probe()
{
    (cd "$BUILD" && SST_REQUIRE_DATAPLANE="$2" ctest --output-on-failure --no-tests=error \
        -R "$PROBE_TEST") >"$RESULTS/probe-$1.log" 2>&1
}

# Configure has to see the real launcher, because the staging tests that kill a reader or writer
# are left out when mpiexec is a script.  So the test drivers are pointed at launch.sh afterwards.
use_launcher()
{
    export QUAL_SCHED QUAL_LAUNCH_NODES="$LAUNCH_NODES" QUAL_LOCKS="$RESULTS/locks" \
        QUAL_STEP_WAIT="${STEP_WAIT:-60}"
    QUAL_MPIEXEC="$(sed -n 's/^MPIEXEC_EXECUTABLE:[A-Z]*=//p' "$BUILD/CMakeCache.txt")"
    export QUAL_MPIEXEC
    case "$QUAL_SCHED" in
    slurm) QUAL_HOSTS="$(scontrol show hostnames "$SLURM_JOB_NODELIST" | xargs)" ;;
    pals) QUAL_HOSTS="$(sort -u "$PBS_NODEFILE" | cut -d. -f1 | xargs)" ;;
    esac
    export QUAL_HOSTS
    mkdir -p "$QUAL_LOCKS"
    sed -i "s#$QUAL_MPIEXEC#$HERE/launch.sh#g" "$BUILD"/bin/run_test.py.* \
        "$BUILD"/bin/run_multi_stream.py.*
    record "launcher ok hosts=$QUAL_HOSTS"
}

do_run()
{
    RESULTS="$ROOT/results/$(date +%Y%m%d-%H%M%S)-$SHA"
    mkdir -p "$RESULTS" || exit 1
    {
        echo "site=$site"
        echo "commit=$(git -C "$SRC" rev-parse HEAD)"
        echo "describe=$(git -C "$SRC" describe --tags --always)"
        echo "tree_changes=$(git -C "$SRC" status --short --untracked-files=no | wc -l | xargs)"
        echo "date=$(date)"
        echo "build=$BUILD"
        # shellcheck disable=SC2086 # one sst- phase per data plane
        echo "phases=${GPU_TESTS:+gpu }${SITE_PHASES:+$SITE_PHASES }$(printf 'sst-%s ' $DATAPLANES)"
    } >"$RESULTS/meta.txt"
    cat "$RESULTS/meta.txt"

    if [ -n "$GPU_TESTS" ]; then
        run_ctest gpu -R "$GPU_TESTS"
        site_settle
    fi

    site_phases

    if [ -n "$DATAPLANES" ]; then
        # The probe must work on its own, or the checks below fail for the wrong reason.
        if ! probe launcher ""; then
            record "launcher FAILED tests cannot start, see probe-launcher.log"
            exit 1
        fi
        # If the requirement does not reach the ranks, every dataplane result below is
        # meaningless, because SST would quietly use whatever it picks by default.
        if probe control "$NO_DATAPLANE" ||
            ! grep -q "SST_REQUIRE_DATAPLANE is \"$NO_DATAPLANE\"" "$RESULTS/probe-control.log"; then
            record "control FAILED SST_REQUIRE_DATAPLANE did not reach the ranks, stopping"
            exit 1
        fi
        record "control ok"
    fi

    if [ -n "$LAUNCH_NODES" ]; then
        use_launcher
    fi

    local dp
    local skip exclude
    for dp in $DATAPLANES; do
        site_settle
        if probe "$dp" "$dp"; then
            # SKIP_<dataplane> names tests that cannot pass here, so they cost no timeouts.
            skip="SKIP_$dp"
            skip="${!skip}"
            exclude="$EXCLUDE"
            if [ -n "$skip" ]; then
                exclude="${exclude:+$exclude|}$skip"
                record "sst-$dp skips $(cd "$BUILD" && ctest -N -R "$SST_TESTS" |
                    grep -E -c "Test +#[0-9]+: ($skip)( |$)") tests matching SKIP_$dp"
            fi
            EXCLUDE="$exclude" SST_REQUIRE_DATAPLANE="$dp" run_ctest "sst-$dp" -R "$SST_TESTS"
            recheck "$dp"
        else
            record "sst-$dp UNAVAILABLE see probe-$dp.log"
        fi
    done
    echo ">> results in $RESULTS"
}

case "$cmd" in
build) do_build ;;
submit)
    if [ ! -d "$BUILD" ] && [ -z "$BUILD_IN_JOB" ]; then
        echo "!! no build at $BUILD, run: qualify.sh $site build" >&2
        exit 1
    fi
    site_submit
    ;;
run) do_run ;;
*)
    echo "unknown command $cmd" >&2
    exit 2
    ;;
esac
