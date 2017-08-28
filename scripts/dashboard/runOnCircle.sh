#!/usr/bin/bash

# echo "runOnCircle.sh (node index = ${CIRCLE_NODE_INDEX}, total nodes = ${CIRCLE_NODE_TOTAL})"

# Initialize modules system
# . /etc/profile.d/lmod.sh >/dev/null

BASEDIR=$(readlink -f $(dirname ${BASH_SOURCE}))
cd ${BASEDIR}

DASHBOARD_CONFIGS="GNU4_NoMPI GNU7_OpenMPI"

mkdir -p ${BASEDIR}/../../../Logs

# ARGUMENTS="$@"

# IFS=' '
# read -ra DASHSCRIPTS <<< "$ARGUMENTS"
# for i in "${DASHSCRIPTS[@]}"; do
for CONFIG in ${DASHBOARD_CONFIGS}
do
    echo ${CONFIG}

    DASHBOARDDIR=${BASEDIR}/../../../${CONFIG}
    mkdir -p "${DASHBOARDDIR}"
    export USE_BASE_DIRECTORY="${DASHBOARDDIR}"

    LOG=${BASEDIR}/../../../Logs/${CONFIG}
    # ctest -S ${BASEDIR}/../dashboard/circle_${CONFIG}.cmake -VV 1>${LOG}.out 2>${LOG}.err
    ctest -S ${BASEDIR}/circle_${CONFIG}.cmake -VV
    
    # if [[ "$i" =~ scripts/dashboard/circle_([^\.]+) ]];
    # then
    #     CONFIG="${BASH_REMATCH[1]}"

        # echo "Running build: ${CONFIG}"

        # DASHBOARDDIR=${BASEDIR}/../../../${CONFIG}

        # echo "Dashboard directory for this build: ${DASHBOARDDIR}"

        # mkdir -p "${DASHBOARDDIR}/ADIOS2"
        # mkdir -p "${DASHBOARDDIR}/Logs"

        # export CTEST_DASHBOARD_ROOT="${DASHBOARDDIR}"

        # DASHBOARDCONFIG="${BASEDIR}/../dashboard/circle_${CONFIG}.cmake"

        # echo "Dashboard script: ${DASHBOARDCONFIG}"

        # LOG="${DASHBOARDDIR}/Logs/${CONFIG}"

        # ctest -S $DASHBOARDCONFIG -VV 1>${LOG}.out 2>${LOG}.err
    # else
    #     echo "Unable to find build name in ${i}"
    # fi
done
