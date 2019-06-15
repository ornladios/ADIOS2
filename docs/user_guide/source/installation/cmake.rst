**************************************************
Building, Testing and Installing ADIOS2 with CMake
**************************************************

CMake uses "out-of-source" builds, which means keeping a separate build and source directory (different from autotools, which usually uses an in-source build).

To build ADIOS2: 

1. Clone the repository: 

.. code-block:: bash 
    
    $ git clone https://github.com/ornladios/ADIOS2.git adios2-source


2. Create a separate build directory: 

.. code-block:: bash 
    
    $ mkdir adios2-build


3. Configure the project with CMake: 

.. code-block:: bash

    $ cd adios2-build 
    $ cmake ../adios2-source
    -- The C compiler identification is GNU 7.3.0
    -- The CXX compiler identification is GNU 7.3.0
    -- Check for working C compiler: /opt/ohpc/pub/compiler/gcc/7.3.0/bin/gcc
    -- Check for working C compiler: /opt/ohpc/pub/compiler/gcc/7.3.0/bin/gcc -- works
    -- Detecting C compiler ABI info
    -- Detecting C compiler ABI info - done
    -- Detecting C compile features
    -- Detecting C compile features - done
    -- Check for working CXX compiler: /opt/ohpc/pub/compiler/gcc/7.3.0/bin/g++
    -- Check for working CXX compiler: /opt/ohpc/pub/compiler/gcc/7.3.0/bin/g++ -- works
    -- Detecting CXX compiler ABI info
    -- Detecting CXX compiler ABI info - done
    -- Detecting CXX compile features
    -- Detecting CXX compile features - done
    -- Found BZip2: /usr/lib64/libbz2.so (found version "1.0.6") 
    -- Looking for BZ2_bzCompressInit
    -- Looking for BZ2_bzCompressInit - found
    -- Could NOT find ZFP (missing: ZFP_LIBRARY ZFP_INCLUDE_DIR) 
    -- Could NOT find SZ (missing: SZ_LIBRARY ZLIB_LIBRARY ZSTD_LIBRARY SZ_INCLUDE_DIR) 
    -- Could NOT find MGARD (missing: MGARD_LIBRARY ZLIB_LIBRARY MGARD_INCLUDE_DIR) 
    -- Looking for a Fortran compiler
    -- Looking for a Fortran compiler - /opt/ohpc/pub/compiler/gcc/7.3.0/bin/gfortran
    -- The Fortran compiler identification is GNU 7.3.0
    -- Check for working Fortran compiler: /opt/ohpc/pub/compiler/gcc/7.3.0/bin/gfortran
    -- Check for working Fortran compiler: /opt/ohpc/pub/compiler/gcc/7.3.0/bin/gfortran  -- works
    -- Detecting Fortran compiler ABI info
    -- Detecting Fortran compiler ABI info - done
    -- Checking whether /opt/ohpc/pub/compiler/gcc/7.3.0/bin/gfortran supports Fortran 90
    -- Checking whether /opt/ohpc/pub/compiler/gcc/7.3.0/bin/gfortran supports Fortran 90 -- yes
    -- Found MPI_C: /opt/ohpc/pub/mpi/openmpi3-gnu7/3.1.0/lib/libmpi.so (found version "3.1") 
    -- Found MPI_Fortran: /opt/ohpc/pub/mpi/openmpi3-gnu7/3.1.0/lib/libmpi_usempif08.so (found version "3.1") 
    -- Found MPI: TRUE (found version "3.1") found components:  C Fortran 
    -- Found ZeroMQ: /usr/lib64/libzmq.so (found suitable version "4.1.4", minimum required is "4.1") 
    -- HDF5: Using hdf5 compiler wrapper to determine C configuration
    -- Found HDF5: /opt/ohpc/pub/libs/gnu7/openmpi3/hdf5/1.10.2/lib/libhdf5.so (found version "1.10.2") found components:  C 
    -- Found PythonInterp: /usr/bin/python3 (found version "3.4.9") 
    -- Found PythonLibs: /usr/lib64/libpython3.4m.so (found version "3.4.9") 
    -- Found PythonModule_numpy: /opt/ohpc/pub/libs/gnu7/numpy/1.14.3/lib64/python3.4/site-packages/numpy  
    -- Found PythonModule_mpi4py: /opt/ohpc/pub/libs/gnu7/openmpi3/mpi4py/3.0.0/lib64/python3.4/site-packages/mpi4py  
    -- Found PythonFull: /usr/bin/python3  found components:  Interp Libs numpy mpi4py 
    -- Found PkgConfig: /usr/bin/pkg-config (found version "0.27.1") 
    -- Checking for module 'libfabric'
    --   No package 'libfabric' found
    -- Could NOT find LIBFABRIC (missing: LIBFABRIC_LIBRARIES) (Required is at least version "1.6")
    -- Looking for shmget
    -- Looking for shmget - found
    -- Looking for pthread.h
    -- Looking for pthread.h - found
    -- Looking for pthread_create
    -- Looking for pthread_create - not found
    -- Looking for pthread_create in pthreads
    -- Looking for pthread_create in pthreads - not found
    -- Looking for pthread_create in pthread
    -- Looking for pthread_create in pthread - found
    -- Found Threads: TRUE  

    -- ADIOS2 ThirdParty: Configuring KWSys
    -- Checking whether header cstdio is available
    -- Checking whether header cstdio is available - yes
    -- Checking for Large File Support
    -- Checking for Large File Support - yes
    -- Checking whether C++ compiler has 'long long'
    -- Checking whether C++ compiler has 'long long' - yes
    -- Checking whether C++ compiler has '__int64'
    -- Checking whether C++ compiler has '__int64' - no
    -- Checking whether wstring is available
    -- Checking whether wstring is available - yes
    -- Checking whether C compiler has ptrdiff_t in stddef.h
    -- Checking whether C compiler has ptrdiff_t in stddef.h - yes
    -- Checking whether C compiler has ssize_t in unistd.h
    -- Checking whether C compiler has ssize_t in unistd.h - yes
    -- Checking whether CXX compiler has setenv
    -- Checking whether CXX compiler has setenv - yes
    -- Checking whether CXX compiler has unsetenv
    -- Checking whether CXX compiler has unsetenv - yes
    -- Checking whether CXX compiler has environ in stdlib.h
    -- Checking whether CXX compiler has environ in stdlib.h - no
    -- Checking whether CXX compiler has utimes
    -- Checking whether CXX compiler has utimes - yes
    -- Checking whether CXX compiler has utimensat
    -- Checking whether CXX compiler has utimensat - yes
    -- Checking whether CXX compiler struct stat has st_mtim member
    -- Checking whether CXX compiler struct stat has st_mtim member - yes
    -- Checking whether CXX compiler struct stat has st_mtimespec member
    -- Checking whether CXX compiler struct stat has st_mtimespec member - no
    -- Checking whether <ext/stdio_filebuf.h> is available
    -- Checking whether <ext/stdio_filebuf.h> is available - yes

    -- ADIOS2 ThirdParty: Configuring GTest
    -- Check if compiler accepts -pthread
    -- Check if compiler accepts -pthread - yes

    -- ADIOS2 ThirdParty: Configuring pybind11
    -- Found PythonLibs: /usr/lib64/libpython3.4m.so
    -- pybind11 v2.2.2

    -- ADIOS2 ThirdParty: Configuring pugixml

    -- ADIOS2 ThirdParty: Configuring nlohmann_json
    -- Using the single-header code from /home/khq.kitware.com/chuck.atkins/Code/adios2/source/master/thirdparty/nlohmann_json/nlohmann_json/single_include/

    -- ADIOS2 ThirdParty: Configuring atl
    -- Looking for sys/types.h
    -- Looking for sys/types.h - found
    -- Looking for stdint.h
    -- Looking for stdint.h - found
    -- Looking for stddef.h
    -- Looking for stddef.h - found
    -- Check size of double
    -- Check size of double - done
    -- Check size of float
    -- Check size of float - done
    -- Check size of int
    -- Check size of int - done
    -- Check size of short
    -- Check size of short - done
    -- Looking for include file malloc.h
    -- Looking for include file malloc.h - found
    -- Looking for include file unistd.h
    -- Looking for include file unistd.h - found
    -- Looking for include file stdlib.h
    -- Looking for include file stdlib.h - found
    -- Looking for include file string.h
    -- Looking for include file string.h - found
    -- Looking for include file sys/time.h
    -- Looking for include file sys/time.h - found
    -- Looking for include file windows.h
    -- Looking for include file windows.h - not found
    -- Looking for fork
    -- Looking for fork - found
    -- Found atl: /home/chuck/adios2-build/thirdparty/atl/atl/atl-config.cmake (found version "2.2.1") 

    -- ADIOS2 ThirdParty: Configuring dill
    -- Check size of void*
    -- Check size of void* - done
    -- Check size of long
    -- Check size of long - done
    -- Check if the system is big endian
    -- Searching 16 bit integer
    -- Check size of unsigned short
    -- Check size of unsigned short - done
    -- Using unsigned short
    -- Check if the system is big endian - little endian
    -- Checking for module 'libffi'
    --   No package 'libffi' found
    -- Could NOT find LibFFI (missing: LIBFFI_LIBRARIES) 
    -- Disabling emulation
    -- Looking for include file stdarg.h
    -- Looking for include file stdarg.h - found
    -- Looking for include file memory.h
    -- Looking for include file memory.h - found
    -- Found dill: /home/chuck/adios2-build/thirdparty/dill/dill/dill-config.cmake (found version "2.4.0") 

    -- ADIOS2 ThirdParty: Configuring ffs
    -- Check size of off_t
    -- Check size of off_t - done
    -- Check size of long double
    -- Check size of long double - done
    -- Check size of long long
    -- Check size of long long - done
    -- Check size of size_t
    -- Check size of size_t - done
    -- Looking for socket
    -- Looking for socket - found
    -- Found BISON: /usr/bin/bison (found version "3.0.4") 
    -- Found FLEX: /usr/bin/flex (found version "2.5.37") 
    -- Found dill: /home/chuck/adios2-build/thirdparty/dill/dill/dill-config.cmake (found suitable version "2.4.0", minimum required is "2.3.1") 
    -- Found atl: /home/chuck/adios2-build/thirdparty/atl/atl/atl-config.cmake (found suitable version "2.2.1", minimum required is "2.2.1") 
    -- Looking for netdb.h
    -- Looking for netdb.h - found
    -- Looking for sockLib.h
    -- Looking for sockLib.h - not found
    -- Looking for sys/select.h
    -- Looking for sys/select.h - found
    -- Looking for sys/socket.h
    -- Looking for sys/socket.h - found
    -- Looking for sys/times.h
    -- Looking for sys/times.h - found
    -- Looking for sys/uio.h
    -- Looking for sys/uio.h - found
    -- Looking for sys/un.h
    -- Looking for sys/un.h - found
    -- Looking for winsock.h
    -- Looking for winsock.h - not found
    -- Looking for strtof
    -- Looking for strtof - found
    -- Looking for strtod
    -- Looking for strtod - found
    -- Looking for strtold
    -- Looking for strtold - found
    -- Looking for getdomainname
    -- Looking for getdomainname - found
    -- Check size of struct iovec
    -- Check size of struct iovec - done
    -- Performing Test HAS_IOV_BASE_IOVEC
    -- Performing Test HAS_IOV_BASE_IOVEC - Success
    -- Found ffs: /home/chuck/adios2-build/thirdparty/ffs/ffs/ffs-config.cmake (found version "1.6.0") 

    -- ADIOS2 ThirdParty: Configuring enet
    -- Looking for getaddrinfo
    -- Looking for getaddrinfo - found
    -- Looking for getnameinfo
    -- Looking for getnameinfo - found
    -- Looking for gethostbyaddr_r
    -- Looking for gethostbyaddr_r - found
    -- Looking for gethostbyname_r
    -- Looking for gethostbyname_r - found
    -- Looking for poll
    -- Looking for poll - found
    -- Looking for fcntl
    -- Looking for fcntl - found
    -- Looking for inet_pton
    -- Looking for inet_pton - found
    -- Looking for inet_ntop
    -- Looking for inet_ntop - found
    -- Performing Test HAS_MSGHDR_FLAGS
    -- Performing Test HAS_MSGHDR_FLAGS - Success
    -- Performing Test HAS_SOCKLEN_T
    -- Performing Test HAS_SOCKLEN_T - Success
    -- Found enet: /home/chuck/adios2-build/thirdparty/enet/enet/enet-config.cmake (found version "1.3.14") 

    -- ADIOS2 ThirdParty: Configuring EVPath
    -- Performing Test HAVE_MATH
    -- Performing Test HAVE_MATH - Failed
    -- Performing Test HAVE_LIBM_MATH
    -- Performing Test HAVE_LIBM_MATH - Success
    -- Found ffs: /home/chuck/adios2-build/thirdparty/ffs/ffs/ffs-config.cmake (found suitable version "1.6.0", minimum required is "1.5.1") 
    -- Could NOT find nvml (missing: NVML_LIBRARY NVML_INCLUDE_DIR) 
    -- Looking for clock_gettime
    -- Looking for clock_gettime - found
    -- Found enet: /home/chuck/adios2-build/thirdparty/enet/enet/enet-config.cmake (found suitable version "1.3.14", minimum required is "1.3.13") 
    --  - Udt4 library was not found.  This is not a fatal error, just that the Udt4 transport will not be built.
    -- Checking for module 'libfabric'
    --   No package 'libfabric' found
    -- Looking for ibv_create_qp
    -- Looking for ibv_create_qp - not found
    -- Looking for ibv_create_qp in ibverbs
    -- Looking for ibv_create_qp in ibverbs - not found
    -- Could NOT find IBVERBS (missing: IBVERBS_LIBRARY) 
    -- Looking for hostlib.h
    -- Looking for hostlib.h - not found
    -- Looking for sys/sockio.h
    -- Looking for sys/sockio.h - not found
    -- Performing Test HAVE_FDS_BITS
    -- Performing Test HAVE_FDS_BITS - Failed
    -- Looking for writev
    -- Looking for writev - found
    -- Looking for uname
    -- Looking for uname - found
    -- Looking for getloadavg
    -- Looking for getloadavg - found
    -- Looking for gettimeofday
    -- Looking for gettimeofday - found
    -- Looking for getifaddrs
    -- Looking for getifaddrs - found
    -- Found EVPath: /home/chuck/adios2-build/thirdparty/EVPath/EVPath/EVPathConfig.cmake (found version "4.4.0") 

    -- Performing Test HAS_FLTO
    -- Performing Test HAS_FLTO - Success
    -- LTO enabled
    -- Detecting Fortran/C Interface
    -- Detecting Fortran/C Interface - Found GLOBAL and MODULE mangling
    -- Verifying Fortran/CXX Compiler Compatibility
    -- Verifying Fortran/CXX Compiler Compatibility - Success
    -- Found MPI: TRUE (found version "3.1") found components:  C 

    ADIOS2 build configuration:
      ADIOS Version: 2.3.1
      C++ Compiler : GNU 7.3.0 
        /opt/ohpc/pub/compiler/gcc/7.3.0/bin/g++

      Fortran Compiler : GNU 7.3.0 
        /opt/ohpc/pub/compiler/gcc/7.3.0/bin/gfortran

      Installation prefix: /usr/local
            bin: bin
            lib: lib
        include: include
          cmake: lib/cmake/adios2
         python: lib/python3.4/site-packages

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
    -- Build files have been written to: /home/chuck/adios2-build
 

4. Compile using ``make -j`` cores

.. code-block:: bash 
    
    $ make -j 16 

5. Run tests with make test or ctest

.. code-block:: bash 
    
    $ ctest
    Test project /home/wgodoy/workspace/build
            Start   1: ADIOSInterfaceWriteTest.DefineVar_int8_t_1x10
      1/295 Test   #1: ADIOSInterfaceWriteTest.DefineVar_int8_t_1x10 .........................   Passed    0.16 sec
            Start   2: ADIOSInterfaceWriteTest.DefineVar_int16_t_1x10
      2/295 Test   #2: ADIOSInterfaceWriteTest.DefineVar_int16_t_1x10 ........................   Passed    0.06 sec
            Start   3: ADIOSInterfaceWriteTest.DefineVar_int32_t_1x10
          
          ...
               
            Start 294: ADIOSBZip2Wrapper.WrongParameterValue
    294/295 Test #294: ADIOSBZip2Wrapper.WrongParameterValue .................................   Passed    0.00 sec
            Start 295: ADIOSBZip2Wrapper.WrongBZip2Name
    295/295 Test #295: ADIOSBZip2Wrapper.WrongBZip2Name ......................................   Passed    0.00 sec
    
    100% tests passed, 0 tests failed out of 295
    
    Total Test time (real) =   95.95 sec
    

5. Install  

.. code-block:: bash 
    
    $ make install


*******************************
Build CMake -DVAR=VALUE Options
*******************************

The following options can be specified with CMake's ``-DVAR=VALUE`` syntax to control which features get enabled or disabled, default option (``ON``/``OFF``) is highlighted:

============================= ========================= ==========================================================================================================================================================================================================================
CMake VAR Option               Values                     Description                                                                     
============================= ========================= ==========================================================================================================================================================================================================================
``ADIOS2_USE_MPI``             **`AUTO`**/``ON``/OFF      MPI or non-MPI (serial) build.                                                                      
``ADIOS2_USE_ZeroMQ``          **`AUTO`**/``ON``/OFF      `ZeroMQ <http://zeromq.org/>`_ for the DataMan engine.                                            
``ADIOS2_USE_HDF5``            **`AUTO`**/``ON``/OFF      `HDF5 <https://www.hdfgroup.org>`_ engine. If HDF5 is not in the path or not the correct version is in the path, set the correct path by the -DHDF5_ROOT=... option      
``ADIOS2_USE_Python``          **`AUTO`**/``ON``/OFF      Python >= 2.7 bindings. mpi4py and numpy. Python 3 will be used if Python 2 and 3 are found. If you want a python version not in the path then choose the right pyhton executable by -DPYTHON_EXECUTABLE=... 
``ADIOS2_USE_Fortran``         **`AUTO`**/``ON``/OFF      Fortran 90 or above bindings. Must have a Fortran compiler. Default is OFF, must be explicitly set to ON.
``ADIOS2_USE_SST``             **`AUTO`**/``ON``/OFF      Simplified Staging Engine (SST) and its dependencies, requires MPI. Can optionally use LibFabric for RDMA transport. Specify the LibFabric install manually with the -DLIBFABRIC_ROOT=... option. 
``ADIOS2_USE_BZIP2``           **`AUTO`**/``ON``/OFF      `BZIP2 <http://www.bzip.org>`_ compression.              
``ADIOS2_USE_ZFP``             **`AUTO`**/``ON``/OFF      `ZFP <https://github.com/LLNL/zfp>`_ compression (experimental).
``ADIOS2_USE_SZ``              **`AUTO`**/``ON``/OFF      `SZ <https://github.com/disheng222/SZ>`_ compression (experimental).
``ADIOS2_USE_MGARD``           **`AUTO`**/``ON``/OFF      `MGARD <https://github.com/CODARcode/MGARD>`_ compression (experimental).
``ADIOS2_USE_ZFP``             **`AUTO`**/``ON``/OFF      `PNG <https://libpng.org>`_ compression (experimental).
``ADIOS2_USE_Endian_Reverse``  **`AUTO`**/ON/``OFF``      Big/Little Endian Interoperability for different endianness platforms at write and read.
============================= ========================= ==========================================================================================================================================================================================================================

Examples: Enable Fortran, disable Python bindings and ZeroMQ functionality 

.. code-block:: bash

    $ cmake -DADIOS2_USE_Fortran=ON -DADIOS2_USE_Python=OFF -DADIOS2_USE_ZeroMQ=OFF ../ADIOS2


Notes: 
   * The ``ADIOS2_USE_HDF5`` option requires the use of a matching serial or parallel version depending on whether ``ADIOS2_USE_MPI`` is enabled. Similary, enabling MPI and Python bindings require ``mpi4py``.
   
   * Optional ROOT suffix to a dependency can guide cmake into finding a particular dependency:
   
.. code-block:: bash

    $ cmake -DHDF5_ROOT=/opt/hdf5/1.12.0 ../ADIOS2

In addition to the ``ADIOS2_USE_Feature`` options, the following options are also available to control how the library gets built:

==================================== =============================================== ===============================
 CMake VAR Options                       Values                                       Description                                                                          |
==================================== =============================================== ===============================
``BUILD_SHARED_LIBS``                  ``ON``/OFF                                     Build shared libraries.                                                               
``ADIOS2_BUILD_EXAMPLES``              ``ON``/OFF                                     Build examples.                                                                       
``ADIOS2_BUILD_TESTING``               ``ON``/OFF                                     Build test code.                                                                      
``CMAKE_INSTALL_PREFIX``               /path/to/install (``/usr/local``)              Installation location.                                                                     
``CMAKE_BUILD_TYPE``                   ``Debug``/Release/RelWithDebInfo/MinSizeRel    Compiler optimization levels.                                            
==================================== =============================================== ===============================

Example: the following configuration will build, test and install under /opt/adios2/2.3.1 an optimized (Release) version of ADIOS2.

.. code-block:: bash

    $ cd build 
    $ cmake -DADIOS2_USE_Fortran=ON -DCMAKE_INSTALL_PREFIX=/opt/adios2/2.3.1 -DCMAKE_BUILD_Type=Release ../ADIOS2
    $ make -j16 
    $ ctest
    $ make install
 
For a full configurable script example, click `here. <https://github.com/ornladios/ADIOS2/tree/master/scripts/runconf/runconf.sh>`_
