#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

# Stands in for mpiexec in the staging tests, so that every separate launch gets nodes of its
# own.  Slingshot systems need this.  On Frontier a second job step on a node fails with "Error
# configuring interconnect".  On Aurora, launches sharing a node on the job VNI can collide on a
# PID and fail with EEXIST.  See README.md.  qualify.sh sets the QUAL_ variables.

# An MPMD launch is a single job step, so it needs no placement.
case " $* " in *" : "*) exec "$QUAL_MPIEXEC" "$@" ;; esac

read -r -a hosts <<<"$QUAL_HOSTS"
groups=$((${#hosts[@]} / QUAL_LAUNCH_NODES))

# The lock is held on fd 9, which the launcher inherits, so it goes away with the launch however
# that ends.  Tests kill their launches, so nothing here can run after the exec.
group=-1
for ((wait = 0; wait < 300 && group < 0; wait++)); do
    for ((g = 0; g < groups; g++)); do
        exec 9>"$QUAL_LOCKS/group$g"
        if flock -n 9; then
            group=$g
            break
        fi
        exec 9>&-
    done
    [ "$group" -ge 0 ] || sleep 1
done
if [ "$group" -lt 0 ]; then
    echo "launch.sh: no free nodes among ${#hosts[@]} after 300 seconds" >&2
    exit 1
fi

mine=("${hosts[@]:$((group * QUAL_LAUNCH_NODES)):QUAL_LAUNCH_NODES}")
nprocs=1
args=("$@")
for ((i = 0; i < ${#args[@]} - 1; i++)); do
    [ "${args[i]}" = "-n" ] && nprocs="${args[i + 1]}"
done

case "$QUAL_SCHED" in
slurm)
    # A killed srun can leave its step behind for a while, and on Frontier that still blocks
    # the node.  The bound is in seconds, since squeue itself can take tens of seconds.
    end=$((SECONDS + ${QUAL_STEP_WAIT:-60}))
    while [ "$SECONDS" -lt "$end" ]; do
        squeue -h -s -j "$SLURM_JOB_ID" -o "%i %N" | grep -v -E '\.(batch|extern|interactive) ' |
            grep -q -w "${mine[0]}" || break
        sleep 1
    done
    exec "$QUAL_MPIEXEC" -w "$(IFS=,; echo "${mine[*]}")" -N "${#mine[@]}" "$@"
    ;;
pals)
    ppn=$(((nprocs + ${#mine[@]} - 1) / ${#mine[@]}))
    exec "$QUAL_MPIEXEC" --hosts "$(IFS=,; echo "${mine[*]}")" --ppn "$ppn" "$@"
    ;;
*)
    echo "launch.sh: unknown scheduler $QUAL_SCHED" >&2
    exit 1
    ;;
esac
