[![GitHub (pre-)release](https://img.shields.io/github/release/ornladios/adios2/all.svg)]()
[![GitHub license](http://dmlc.github.io/img/apache2.svg)](./LICENSE)


[![Circle CI](https://circleci.com/gh/ornladios/ADIOS2.svg?style=shield)](https://circleci.com/gh/ornladios/ADIOS2)
[![Travis CI](https://travis-ci.org/ornladios/ADIOS2.svg)](https://travis-ci.org/ornladios/ADIOS2)
[![AppVeyor CI](https://ci.appveyor.com/api/projects/status/0s2a3qp57hgbvlhj?svg=true)](https://ci.appveyor.com/project/ornladios/adios2)

[![Coverity Scan Build Status](https://scan.coverity.com/projects/11116/badge.svg)](https://scan.coverity.com/projects/ornladios-adios2)

# Adaptable Input / Output System (ADIOS) v2.1.0
This is v2.1.0 of the ADIOS I/O system, developed as part of the
U.S. Department of Energy Exascale Computing Program.

## License
ADIOS >= 2.0 is licensed under the Apache License v2.0.  See the accompanying
Copyright.txt for more details.

## Directory layout

* cmake - Project specific CMake modules
* examples - ADIOS2 Examples
* scripts - Project maintenance and development scripts
* source - Main ADIOS2 source  
    * adios2 - source directory for the ADIOS2 library to be installed    under install-dir/lib/libadios2.  
    * utils  - source directory for the binary utilities, to be installed under install-dir/bin  
* bindings - Additional language bindings (C, Fortran and Python)
* testing - Tests

## Documentation
Please find ADIOS2 software documentation online under the project GitHub wiki:
[https://github.com/ornladios/ADIOS2/wiki](https://github.com/ornladios/ADIOS2/wiki)

To generate Doxygen application programming interface (API) documentation see 
instructions under: [doc/ReadMe.md](doc/ReadMe.md)  

## Getting Started

ADIOS2 uses CMake for its build environment.  CMake expects projects
to use "out-of-source" builds, which means keeping a separate build and source
directory (different from autotools, which usually uses an in-source build).

To build ADIOS2:

1. Clone the repository:

```bash
$ mkdir adios2
$ cd adios2
$ git clone https://github.com/ornladios/adios2.git source
```

2. Create a separate build directory:

```bash
$ mkdir build
```

3. Configure the project with CMake:

```bash
$ cd build
$ cmake -DCMAKE_INSTALL_PREFIX=/opt/adios2/2.1.0/gnu/openmpi ../source
-- The C compiler identification is GNU 6.3.1
-- The CXX compiler identification is GNU 6.3.1
...

ADIOS2 build configuration:
  ADIOS Version: 2.1.0
  C++ Compiler : GNU 6.3.1
    /usr/bin/c++

  Installation prefix: /opt/adios2/2.0.0/gnu/openmpi
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
      Start  1: ADIOSInterfaceWriteTest.DefineVarChar1x10
 1/31 Test  #1: ADIOSInterfaceWriteTest.DefineVarChar1x10 ..............   Passed    0.00 sec
      Start  2: ADIOSInterfaceWriteTest.DefineVarShort1x10
 2/31 Test  #2: ADIOSInterfaceWriteTest.DefineVarShort1x10 .............   Passed    0.00 sec
...
      Start 21: HDF5WriteReadTest.ADIOS2HDF5WriteHDF5Read1D8
21/31 Test #21: HDF5WriteReadTest.ADIOS2HDF5WriteHDF5Read1D8 ...........   Passed    0.01 sec
      Start 22: HDF5WriteReadTest.ADIOS2HDF5WriteADIOS2HDF5Read1D8
22/31 Test #22: HDF5WriteReadTest.ADIOS2HDF5WriteADIOS2HDF5Read1D8 .....***Not Run (Disabled)   0.00 sec
      Start 23: HDF5WriteReadTest.HDF5WriteADIOS2HDF5Read1D8
23/31 Test #23: HDF5WriteReadTest.HDF5WriteADIOS2HDF5Read1D8 ...........***Not Run (Disabled)   0.00 sec
...
      Start 30: PythonBPWrite
30/31 Test #30: PythonBPWrite ..........................................   Passed    0.12 sec
      Start 31: XMLConfigTest.TwoIOs
31/31 Test #31: XMLConfigTest.TwoIOs ...................................   Passed    0.01 sec

100% tests passed, 0 tests failed out of 25

Total Test time (real) =   0.29 sec

The following tests did not run:
	 22 - HDF5WriteReadTest.ADIOS2HDF5WriteADIOS2HDF5Read1D8 (Disabled)
	 23 - HDF5WriteReadTest.HDF5WriteADIOS2HDF5Read1D8 (Disabled)
	 25 - HDF5WriteReadTest.ADIOS2HDF5WriteADIOS2HDF5Read2D2x4 (Disabled)
	 26 - HDF5WriteReadTest.HDF5WriteADIOS2HDF5Read2D2x4 (Disabled)
	 28 - HDF5WriteReadTest.ADIOS2HDF5WriteADIOS2HDF5Read2D4x2 (Disabled)
	 29 - HDF5WriteReadTest.HDF5WriteADIOS2HDF5Read2D4x2 (Disabled)
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
-- Installing: /opt/adios2/2.1.0/gnu/openmpi/include/adios2/ADIOSConfig.h
...
-- Installing: /opt/adios2/2.1.0/gnu/openmpi/bin/adios2-config
...
-- Installing: /opt/adios2/2.1.0/gnu/openmpi/include/adios2.h
...
-- Installing: /opt/adios2/2.1.0/gnu/openmpi/lib/libadios2.so.2.0.0
-- Installing: /opt/adios2/2.1.0/gnu/openmpi/lib/libadios2.so.2
-- Installing: /opt/adios2/2.1.0/gnu/openmpi/lib/libadios2.so
...
$
```
