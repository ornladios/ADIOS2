[![GitHub (pre-)release](https://img.shields.io/github/release/ornladios/adios2/all.svg)]()
[![GitHub license](http://dmlc.github.io/img/apache2.svg)](./LICENSE)


[![Circle CI](https://circleci.com/gh/ornladios/ADIOS2.svg?style=shield)](https://circleci.com/gh/ornladios/ADIOS2)
[![Travis CI](https://travis-ci.org/ornladios/ADIOS2.svg)](https://travis-ci.org/ornladios/ADIOS2)
[![AppVeyor CI](https://ci.appveyor.com/api/projects/status/0s2a3qp57hgbvlhj?svg=true)](https://ci.appveyor.com/project/ornladios/adios2)

[![Coverity Scan Build Status](https://scan.coverity.com/projects/11116/badge.svg)](https://scan.coverity.com/projects/ornladios-adios2)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/6eeb5a8ac3e34d2599cfdea5bdc3390f)](https://www.codacy.com/app/chuckatkins/ADIOS2?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=ornladios/ADIOS2&amp;utm_campaign=Badge_Grade)


# Adaptable Input / Output System (ADIOS) v2.1.1
This is v2.1.1 of the ADIOS I/O system, developed as part of the
U.S. Department of Energy Exascale Computing Program.

## License
ADIOS >= 2.0 is licensed under the Apache License v2.0.  See the accompanying
Copyright.txt for more details.

## Documentation
Please find ADIOS2 software documentation for:

1. [ADIOS2 User Guide at readthedocs](http://adios2-adaptable-io-system-version-2.readthedocs.io/en/latest/index.html)

2. Local user guide and api documentation generation [docs/ReadMe.md](docs/ReadMe.md)

## Directory layout
* cmake - Project specific CMake modules
* examples - ADIOS2 Examples
* scripts - Project maintenance and development scripts
* source - Main ADIOS2 source  
    * adios2 - source directory for the ADIOS2 library to be installed    under install-dir/lib/libadios2.  
    * utils  - source directory for the binary utilities, to be installed under install-dir/bin  
* bindings - Additional language bindings (C, Fortran and Python)
* testing - Tests
  

## Getting Started

ADIOS2 uses CMake for its build environment.  CMake expects projects
to use "out-of-source" builds, which means keeping a separate build and source
directory (different from autotools, which usually uses an in-source build).

The following is a quick step-by-step build guide, find the full CMake-based install documentation [http://adios2-adaptable-io-system-version-2.readthedocs.io/en/latest/installation/installation.html](here)

Step-by-step build guide:

1. Clone the repository:

```bash
$ mkdir adios2
$ cd adios2
$ git clone https://github.com/ornladios/adios2.git source
```

2. Create a separate build directory in your work area:

```bash
$ mkdir build
```

3. Configure the project with CMake:

```bash
$ cd build
$ cmake -DCMAKE_INSTALL_PREFIX=/opt/adios2/2.1.1/gnu/openmpi ../source
-- The C compiler identification is GNU 6.3.1
-- The CXX compiler identification is GNU 6.3.1
...

ADIOS2 build configuration:
  ADIOS Version: 2.1.1
  C++ Compiler : GNU 6.3.1
    /usr/bin/c++

  Installation prefix: /opt/adios2/2.1.1/gnu/openmpi
  Features:
    Library Type: shared
    Build Type:   Debug
    Testing: ON
    Build Options:
      BZip2    : ON
      ZFP      : OFF
      MPI      : ON
      DataMan  : ON
      ZeroMQ   : ON
      HDF5     : ON
      ADIOS1   : OFF
      Python   : ON
      C        : ON
      SysVShMem: ON

-- Configuring done
-- Generating done
-- Build files have been written to: /home/chuck/Code/adios2/build
$
```

The following options can be specified with CMake's `-DVAR=VALUE` syntax to control which features get enabled or disabled:

| CMake Option         | Values              | Description                                                                      |
| :------------------- | :-------------------------: | :------------------------------------------------------------------------------- |
| `ADIOS2_USE_BZip2`   | **`AUTO`**/``ON``/``OFF`` | Enable [BZip2](http://www.bzip.org/) compression (not implemented).              |
| `ADIOS2_USE_ZFP`     | **`AUTO`**/``ON``/``OFF`` | Enable [ZFP](https://github.com/LLNL/zfp) compression (not implemented).         |
| `ADIOS2_USE_MPI`     | **`AUTO`**/``ON``/``OFF`` | Enable MPI.                                                                      |
| `ADIOS2_USE_DataMan` | **`AUTO`**/``ON``/``OFF`` | Enable the DataMan engine for WAN transports.                                    |
| `ADIOS2_USE_ZeroMQ`  | **`AUTO`**/``ON``/``OFF`` | Enable ZeroMQ for the DataMan engine.                                            |
| `ADIOS2_USE_HDF5`    | **`AUTO`**/``ON``/``OFF`` | Enable the [HDF5](https://www.hdfgroup.org) engine.                              |
| `ADIOS2_USE_ADIOS1`  | **`AUTO`**/``ON``/``OFF`` | Enable the [ADIOS 1.x](https://www.olcf.ornl.gov/center-projects/adios/) engine. |
| `ADIOS2_USE_Python`  | **`AUTO`**/``ON``/``OFF`` | Enable the Python >= 2.7 bindings. |
| `ADIOS2_USE_SST`  | **`AUTO`**/``ON``/``OFF`` | Enable Staging Engine |

Note: The `ADIOS2_USE_HDF5` and `ADIOS2_USE_ADIOS1` options require the use of a matching serial or parallel version depending on whether `ADIOS2_USE_MPI` is enabled.  Similary, enabling MPI and Python bindings requires the presence of `mpi4py`.

In addition to the `ADIOS2_USE_Feature` options, the following options are also available to control how the library get's built:

| CMake Options              | Values                                                    | Description                                                                           |
| :------------------------- | :-------------------------------------------------------: | :------------------------------------------------------------------------------------ |
| `ADIOS2_BUILD_SHARED_LIBS` | **`ON`**/`OFF`                                            | Build shared libraries.                                                               |
| `ADIOS2_ENABLE_PIC`        | **`ON`**/`OFF`                                            | Enable Position Independent Code for static libraries.                                |
| `ADIOS2_BUILD_EXAMPLES`    | **`ON`**/`OFF`                                            | Build examples.                                                                       |
| `ADIOS2_BUILD_TESTING`     | **`ON`**/`OFF`                                            | Build test code.                                                                      |
| `CMAKE_INSTALL_PREFIX`     | /path/to/install (`/usr/local`)                           | Install location.                                                                     |
| `CMAKE_BUILD_TYPE`         | **`Debug`** / `Release` / `RelWithDebInfo` / `MinSizeRel` | The level of compiler optimization to use.                                            |

4. Compile:

```bash
$ make -j8
```

5. Run tests:

```bash
$ ctest
Test project /home/chuck/Code/adios2/build
      Test project /home/wfg/workspace/build
        Start   1: ADIOSInterfaceWriteTest.DefineVar_int8_t_1x10
  1/113 Test   #1: ADIOSInterfaceWriteTest.DefineVar_int8_t_1x10 ............................   Passed    0.07 sec
        Start   2: ADIOSInterfaceWriteTest.DefineVar_int16_t_1x10
  2/113 Test   #2: ADIOSInterfaceWriteTest.DefineVar_int16_t_1x10 ...........................   Passed    0.07 sec
        Start   3: ADIOSInterfaceWriteTest.DefineVar_int32_t_1x10
  3/113 Test   #3: ADIOSInterfaceWriteTest.DefineVar_int32_t_1x10 ...........................   Passed    0.07 sec
        Start   4: ADIOSInterfaceWriteTest.DefineVar_int64_t_1x10
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
-- Install configuration: "Debug"
-- Installing: /opt/adios2/2.1.1/gnu/openmpi/include/adios2/ADIOSConfig.h
...
-- Installing: /opt/adios2/2.1.1/gnu/openmpi/bin/adios2-config
...
-- Installing: /opt/adios2/2.1.1/gnu/openmpi/include/adios2.h
...
-- Installing: /opt/adios2/2.1.1/gnu/openmpi/lib/libadios2.so.2.0.0
-- Installing: /opt/adios2/2.1.1/gnu/openmpi/lib/libadios2.so.2
-- Installing: /opt/adios2/2.1.1/gnu/openmpi/lib/libadios2.so
...
$
```
