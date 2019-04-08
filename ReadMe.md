[![GitHub (pre-)release](https://img.shields.io/github/release/ornladios/adios2/all.svg)]()
[![GitHub license](http://dmlc.github.io/img/apache2.svg)](./LICENSE)


[![Circle CI](https://circleci.com/gh/ornladios/ADIOS2.svg?style=shield)](https://circleci.com/gh/ornladios/ADIOS2)
[![Travis CI](https://travis-ci.org/ornladios/ADIOS2.svg)](https://travis-ci.org/ornladios/ADIOS2)
[![AppVeyor CI](https://ci.appveyor.com/api/projects/status/0s2a3qp57hgbvlhj?svg=true)](https://ci.appveyor.com/project/ornladios/adios2)

[![Coverity Scan Build Status](https://scan.coverity.com/projects/11116/badge.svg)](https://scan.coverity.com/projects/ornladios-adios2)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/6eeb5a8ac3e34d2599cfdea5bdc3390f)](https://www.codacy.com/app/chuckatkins/ADIOS2?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=ornladios/ADIOS2&amp;utm_campaign=Badge_Grade)


# Adaptable Input / Output System (ADIOS) v2.3.1
This is v2.3.1 of the ADIOS I/O system, developed as part of the
U.S. Department of Energy Exascale Computing Program.

## License
ADIOS >= 2.0 is licensed under the Apache License v2.0.  See the accompanying
Copyright.txt for more details.

## Documentation
Please find [ADIOS2 User Guide at readthedocs](https://adios2.readthedocs.io)

## Directory layout
* cmake - Project specific CMake modules
* examples - ADIOS2 Examples
* scripts - Project maintenance and development scripts
* source - ADIOS2 source  
    * adios2 - source directory for the ADIOS2 library to be installed under install-dir/lib/libadios2.  
    * utils  - source directory for the binary utilities, to be installed under install-dir/bin  
* bindings - public interface language bindings (C++11, C++98, C, Fortran, Python and Matlab)
* testing - Tests using [gtest](https://github.com/google/googletest)


## Getting Started

ADIOS2 uses CMake for its build environment.  CMake expects projects
to use "out-of-source" builds, which means keeping a separate build and source
directory (different from autotools, which usually uses an in-source build).

The following is a quick step-by-step build guide, find the full CMake-based install documentation [here](http://adios2.readthedocs.io/en/latest/installation/installation.html)

Step-by-step build guide:

1. Clone the repository:

```bash
$ git clone https://github.com/ornladios/ADIOS2.git
```

2. Create a separate build directory in your work area:

```bash
ADIOS2$ mkdir build && cd build
```

3. Configure the project with CMake:

```bash
ADIOS2/build$ cmake -DCMAKE_INSTALL_PREFIX=/opt/adios2/2.3.1/gnu/openmpi ../
-- The C compiler identification is GNU 7.3.0
-- The CXX compiler identification is GNU 7.3.0
...

ADIOS2 build configuration:
  ADIOS Version: 2.3.1
  C++ Compiler : GNU 7.3.0
    /opt/ohpc/pub/compiler/gcc/7.3.0/bin/g++

  Fortran Compiler : GNU 7.3.0
    /opt/ohpc/pub/compiler/gcc/7.3.0/bin/gfortran

  Installation prefix: /opt/adios2/2.3.1/gnu/openmpi
        bin: bin
        lib: lib
    include: include
      cmake: lib/cmake/adios2

  Features:
    Library Type: shared
    Build Type:   Release
    Testing: ON
    Build Options:
      BZip2    : ON
      ZFP      : OFF
      SZ       : OFF
      MGARD    : OFF
      MPI      : ON
      DataMan  : ON
      SST      : ON
      ZeroMQ   : ON
      HDF5     : ON
      Python   : ON
      Fortran  : ON
      SysVShMem: ON
      Endian_Reverse: OFF

-- Configuring done
-- Generating done
-- Build files have been written to: /home/chuck/ADIOS2/build

```

The following options can be specified with CMake's `-DVAR=VALUE` syntax to control which features get enabled or disabled:

| CMake Option         | Values                    | Description                                                              |
| :------------------- | :------------------------ | :----------------------------------------------------------------------- |
| `ADIOS2_USE_BZip2`   | **`AUTO`**/``ON``/``OFF`` | Enable [BZip2](http://www.bzip.org/) compression (not implemented).      |
| `ADIOS2_USE_ZFP`     | **`AUTO`**/``ON``/``OFF`` | Enable [ZFP](https://github.com/LLNL/zfp) compression (not implemented). |
| `ADIOS2_USE_MPI`     | **`AUTO`**/``ON``/``OFF`` | Enable MPI.                                                              |
| `ADIOS2_USE_DataMan` | **`AUTO`**/``ON``/``OFF`` | Enable the DataMan engine for WAN transports.                            |
| `ADIOS2_USE_ZeroMQ`  | **`AUTO`**/``ON``/``OFF`` | Enable ZeroMQ for the DataMan engine.                                    |
| `ADIOS2_USE_HDF5`    | **`AUTO`**/``ON``/``OFF`` | Enable the [HDF5](https://www.hdfgroup.org) engine.                      |
| `ADIOS2_USE_Python`  | **`AUTO`**/``ON``/``OFF`` | Enable the Python >= 2.7 bindings. Need mpi4py and numpy                 |
| `ADIOS2_USE_SST`     | **`AUTO`**/``ON``/``OFF`` | Enable Staging Engine                                                    |
| `ADIOS2_USE_Fortran` | **`AUTO`**/``ON``/``OFF`` | Enable Fortran bindings                                                  |

Note: The `ADIOS2_USE_HDF5` option requires the use of a matching serial or parallel version depending on whether `ADIOS2_USE_MPI` is enabled.  Similary, enabling MPI and Python bindings requires the presence of `mpi4py`.

In addition to the `ADIOS2_USE_Feature` options, the following options are also available to control how the library get's built:

| CMake Options          | Values                                                    | Description                                |
| :--------------------- | :-------------------------------------------------------- | :----------------------------------------- |
| `BUILD_SHARED_LIBS`    | **`ON`**/`OFF`                                            | Build shared libraries.                    |
| `ADIOS2_BUILD_EXAMPLE` | **`ON`**/`OFF`                                            | Build examples.                            |
| `ADIOS2_BUILD_TESTING` | **`ON`**/`OFF`                                            | Build test code.                           |
| `CMAKE_INSTALL_PREFIX` | /path/to/install (`/usr/local`)                           | Install location.                          |
| `CMAKE_BUILD_TYPE`     | **`Debug`** / `Release` / `RelWithDebInfo` / `MinSizeRel` | The level of compiler optimization to use. |

4. Compile:

```bash
ADIOS2/build$ make -j8
```

5. Run tests:

```bash
$ ctest
Test project /home/chuck/adios2/build
       Start   1: HeatTransfer.BPFile.Write.MxM
  1/295 Test   #1: HeatTransfer.BPFile.Write.MxM ............................................   Passed    1.25 sec
        Start   2: HeatTransfer.BPFile.Read.MxM
  2/295 Test   #2: HeatTransfer.BPFile.Read.MxM .............................................   Passed    0.55 sec
        Start   3: HeatTransfer.BPFile.Dump.MxM
  ...

$
```

6.  Install:
```
$ make install
[  7%] Built target adios2sys_objects
...
[ 61%] Built target adios2
[ 68%] Built target adios2py
...
Install the project...
-- Install configuration: "Release"
-- Installing: /opt/adios2/2.3.1/gnu/openmpi/include/adios2/ADIOSConfig.h
...
-- Installing: /opt/adios2/2.3.1/gnu/openmpi/bin/adios2-config
...
-- Installing: /opt/adios2/2.3.1/gnu/openmpi/include/adios2.h
...
-- Installing: /opt/adios2/2.3.1/gnu/openmpi/lib/libadios2.so.2.3.1
-- Installing: /opt/adios2/2.3.1/gnu/openmpi/lib/libadios2.so.2
-- Installing: /opt/adios2/2.3.1/gnu/openmpi/lib/libadios2.so
...
$
```
