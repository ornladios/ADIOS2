#!/bin/bash -e

ranks=4
ranks_x=2
ranks_y=2
run_reader() {
    mpirun -np $ranks $* \
        `pwd`/build/bin/heatTransfer_read \
        `pwd`/examples/heatTransfer/heat_sst_bp.xml \
        HeatTransfer.SST.BP.Write.MxM.bp \
        HeatTransfer.SST.BP.Read.MxM $ranks_x $ranks_y &
}

run_writer() {
    export TAU_TRACK_SIGNALS=1
    mpiexec -np $ranks $* \
        `pwd`/build/bin/heatTransfer_write_adios2 \
        `pwd`/examples/heatTransfer/heat_sst_bp.xml \
        HeatTransfer.SST.BP.Write.MxM $ranks_x $ranks_y 10 10 1000 100
}

original() {
    run_reader
    run_writer
}

profile() {
    export PROFILEDIR=reader_profiles
    rm -rf $PROFILEDIR
    mkdir $PROFILEDIR
    run_reader tau_exec -T mpi,pthread -io

    export PROFILEDIR=writer_profiles
    rm -rf $PROFILEDIR
    mkdir $PROFILEDIR
    run_writer tau_exec -T mpi,pthread -io

    unset PROFILEDIR
}

profile_xml() {
    export TAU_EBS_UNWIND=1
    export TAU_EBS_KEEP_UNRESOLVED_ADDR=1
    export TAU_PROFILE_FORMAT=merged
    export TAU_PROFILE_PREFIX=reader
    run_reader tau_exec -T mpi,pthread -io -ebs

    export TAU_PROFILE_PREFIX=writer
    run_writer tau_exec -T mpi,pthread -io -ebs

    unset TAU_PROFILE_FORMAT
    unset TAU_PROFILE_PREFIX
    unset TAU_EBS_UNWIND
    unset TAU_EBS_KEEP_UNRESOLVED_ADDR
}

trace() {
    export TAU_TRACE=1
    export TRACEDIR=`pwd`/reader_trace
    rm -rf $TRACEDIR
    mkdir $TRACEDIR
    run_reader tau_exec -T mpi,pthread -io

    export TRACEDIR=`pwd`/writer_trace
    rm -rf $TRACEDIR
    mkdir $TRACEDIR
    run_writer tau_exec -T mpi,pthread -io

    unset TAU_TRACING
    unset TRACEDIR
    wait
    sleep 1

    # Merge and convert the reader trace
    cd `pwd`/reader_trace
    tau_treemerge.pl
    tau2slog2 tau.trc tau.edf -o tau.slog2
    cd ..

    # Merge and convert the writer trace
    cd `pwd`/writer_trace
    tau_treemerge.pl
    tau2slog2 tau.trc tau.edf -o tau.slog2
    cd ..
}

trace_otf2() {
    export TAU_TRACE=1
    export TAU_TRACE_FORMAT=otf2
    export TRACEDIR=`pwd`/reader_trace_otf2
    rm -rf $TRACEDIR
    mkdir $TRACEDIR
    run_reader tau_exec -T mpi,pthread -io

    export TRACEDIR=`pwd`/writer_trace_otf2
    rm -rf $TRACEDIR
    mkdir $TRACEDIR
    run_writer tau_exec -T mpi,pthread -io

    unset TAU_TRACE
    unset TAU_TRACE_FORMAT
    unset TRACEDIR
}

export TAU_CALLPATH=1
export TAU_CALLPATH_DEPTH=100
rm -rf HeatTransfer.SST.BP.Read.MxM.bp*
#original
#profile
profile_xml
#trace
#trace_otf2

