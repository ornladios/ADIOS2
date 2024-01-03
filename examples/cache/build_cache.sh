#!/bin/bash

# This script is to build the remote server with cache enabled.
# Attention: hiredis cannot be installed by apt-get, since the default version is too old (libhiredis0.14 (= 0.14.1-2)).
#            We need to build it from source code. You can also use the following scripts to install hiredis (v1.2.0).

# sample usage: in project home directory:
# source examples/cache/build_cache.sh --build 
# source examples/cache/build_cache.sh --start
# source examples/cache/build_cache.sh --stop

if [ -z ${BUILD_DIR} ]
then
    BUILD_DIR=${PWD}/build-cache-test
fi

if [ ! -d ${BUILD_DIR} ]
then
    mkdir -p ${BUILD_DIR}
fi

SW_DIR=${BUILD_DIR}/sw
if [ ! -d ${SW_DIR} ]
then
    mkdir -p ${SW_DIR}
fi

build_cache() {
    # redis - in-memory data structure store
    redis_dir=${SW_DIR}/redis
    if [ ! -d ${redis_dir} ]
    then
        git clone https://github.com/redis/redis.git ${redis_dir}
        cd ${redis_dir}
        git checkout tags/7.2.3
        # cannot accleerate by 'make -j8'. It will cause error.
        make

        # hiredis - C client library to connect Redis server
        cd ${redis_dir}/deps/hiredis
        mkdir build && cd build
        cmake .. -DCMAKE_INSTALL_PREFIX=${SW_DIR}/hiredis
        make -j32
        make install
    fi

    cd ${BUILD_DIR}
    cmake .. -DADIOS2_USE_Cache=ON \
            -DADIOS2_USE_Python=ON \
            -DCMAKE_PREFIX_PATH=${SW_DIR} \
            -DCMAKE_INSTALL_PREFIX=${SW_DIR}/adios2

    make -j32
    make install
    cd ${BUILD_DIR}/../
}

start_services() {
    echo "Starting redis server and setting environment variables..."
    export DoRemote=1
    export useKVCache=1
    export PYTHONPATH=${SW_DIR}/adios2/local/lib/python3.10/dist-packages/
    export LD_LIBRARY_PATH=${SW_DIR}/adios2/lib:${SW_DIR}/hiredis/lib:$LD_LIBRARY_PATH
    nohup ${SW_DIR}/redis/src/redis-server > ${SW_DIR}redis_server.log 2>&1 &
    nohup ${SW_DIR}/adios2/bin/adios2_remote_server > ${SW_DIR}remote_server.log 2>&1 &
    sleep 5
    nohup ${SW_DIR}/redis/src/redis-cli monitor > ${SW_DIR}redis_monitor.log 2>&1 &
    echo "Services started and environment variables set."
}

# Function to stop services (optional, example purpose)
stop_services() {
    echo "Stopping services..."
    pkill -f redis-server
    pkill -f redis-cli
    pkill -f remote_server
    unset DoRemote
    unset useKVCache
    unset PYTHONPATH
    unset LD_LIBRARY_PATH
    echo "Services stopped."
}

# Parse command line options
while [[ "$1" != "" ]]; do
    case "$1" in
        -b | --build )
            build_cache
            ;;
        -c | --start )
            start_services
            ;;
        -s | --stop )
            stop_services
            ;;
        -h | --help )
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  -b, --build   Build the software with cache enabled"
            echo "  -c, --start   Start redis server and set environment variables"
            echo "  -s, --stop    Stop the services"
            echo "  -h, --help    Display this help message"
            ;;
        * )
            echo "Invalid option: $1"
            echo "Use -h or --help for usage information."
            ;;
    esac
    shift
done
