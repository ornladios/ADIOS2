**************************************************
Building, Testing and Installing ADIOS2 with CMake
**************************************************

CMake uses "out-of-source" builds, which means keeping a separate build and source directory (different from autotools, which usually uses an in-source build).

To build ADIOS2: 

1. Clone the repository: 

.. code-block:: bash 
    
    $ git clone https://github.com/ornladios/ADIOS2.git 


2. Create a separate build directory: 

.. code-block:: bash 
    
    $ mkdir build 


3. Configure the project with CMake: 

.. code-block:: bash

    $ cd build 
    $ cmake -DADIOS2_USE_Fortran=ON ../ADIOS2 
    -- The C compiler identification is GNU 5.4.0
    -- The CXX compiler identification is GNU 5.4.0
    -- Check for working C compiler: /usr/bin/cc
    -- Check for working C compiler: /usr/bin/cc -- works
    -- Detecting C compiler ABI info
    -- Detecting C compiler ABI info - done
    -- Detecting C compile features
    -- Detecting C compile features - done
    -- Check for working CXX compiler: /usr/bin/c++
    -- Check for working CXX compiler: /usr/bin/c++ -- works
    -- Detecting CXX compiler ABI info
    -- Detecting CXX compiler ABI info - done
    -- Detecting CXX compile features
    -- Detecting CXX compile features - done
    -- Found BZip2: /usr/lib/x86_64-linux-gnu/libbz2.so (found version "1.0.6") 
    -- Looking for BZ2_bzCompressInit
    -- Looking for BZ2_bzCompressInit - found
    -- Could NOT find ZFP (missing:  ZFP_LIBRARY ZFP_INCLUDE_DIR) 
    -- The Fortran compiler identification is GNU 5.4.0
    -- Check for working Fortran compiler: /usr/bin/gfortran
    -- Check for working Fortran compiler: /usr/bin/gfortran  -- works
    -- Detecting Fortran compiler ABI info
    -- Detecting Fortran compiler ABI info - done
    -- Checking whether /usr/bin/gfortran supports Fortran 90
    -- Checking whether /usr/bin/gfortran supports Fortran 90 -- yes
    -- Found MPI_C: /usr/lib/openmpi/lib/libmpi.so (found version "3.0") 
    -- Found MPI_Fortran: /usr/lib/openmpi/lib/libmpi_usempif08.so (found version "3.0") 
    -- Found MPI: TRUE (found version "3.0") found components:  C Fortran 
    -- Found ZeroMQ: /usr/lib/x86_64-linux-gnu/libzmq.so  
    -- HDF5: Using hdf5 compiler wrapper to determine C configuration
    -- Found HDF5: /usr/lib/x86_64-linux-gnu/hdf5/openmpi/libhdf5.so;/usr/lib/x86_64-linux-gnu/libsz.so;/usr/lib/x86_64-linux-gnu/libz.so;/usr/lib/x86_64-linux-gnu/libdl.so;/usr/lib/x86_64-linux-gnu/libm.so (found version "1.8.16") found components:  C 
    -- Looking for pthread.h
    -- Looking for pthread.h - found
    -- Looking for pthread_create
    -- Looking for pthread_create - not found
    -- Looking for pthread_create in pthreads
    -- Looking for pthread_create in pthreads - not found
    -- Looking for pthread_create in pthread
    -- Looking for pthread_create in pthread - found
    -- Found Threads: TRUE  
    -- Could NOT find ADIOS1 (missing:  ADIOS1_LIBRARY ADIOS1_INCLUDE_DIR) (Required is at least version "1.12.0")
    -- Found PythonInterp: /usr/bin/python3 (found version "3.5.2") 
    -- Found PythonLibs: /usr/lib/x86_64-linux-gnu/libpython3.5m.so
    -- Found PythonModule_numpy: /usr/lib/python3/dist-packages/numpy  
    -- Found PythonModule_mpi4py: /usr/lib/python3/dist-packages/mpi4py  
    -- Found PythonFull: /usr/bin/python3  found components:  Interp Libs numpy mpi4py 
    -- Could NOT find EVPath (missing:  EVPath_LIBRARY EVPath_INCLUDE_DIR) 
    -- Looking for shmget
    -- Looking for shmget - found
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
    -- pybind11 v2.2.1
    -- Performing Test HAS_FLTO
    -- Performing Test HAS_FLTO - Success
    -- LTO enabled
    -- Detecting Fortran/C Interface
    -- Detecting Fortran/C Interface - Found GLOBAL and MODULE mangling
    -- Verifying Fortran/CXX Compiler Compatibility
    -- Verifying Fortran/CXX Compiler Compatibility - Success
    -- Found MPI: TRUE (found version "3.0") found components:  C 
    
    ADIOS2 build configuration:
      ADIOS Version: 2.2.0
      C++ Compiler : GNU 5.4.0 
        /usr/bin/c++
    
      Fortran Compiler : GNU 5.4.0 
        /usr/bin/gfortran
    
      Installation prefix: /usr/local
            bin: bin
            lib: lib
        include: include
          cmake: lib/cmake/adios2
         python: lib/python3.5/site-packages
    
      Features:
        Library Type: shared
        Build Type:   Debug
        Testing: ON
        Build Options:
          BZip2    : ON
          ZFP      : OFF
          MPI      : ON
          DataMan  : ON
          SST      : OFF
          ZeroMQ   : ON
          HDF5     : ON
          ADIOS1   : OFF
          Python   : ON
          Fortran  : ON
          SysVShMem: ON
    
    -- Configuring done
    -- Generating done
    -- Build files have been written to: /home/wgodoy/workspace/build  
 

4. Compile using make -j cores

.. code-block:: bash 
    
    $ make -j 4 

5. Run tests with make test or ctest

.. code-block:: bash 
    
    $ ctest
    Test project /home/wgodoy/workspace/build
          Start  1: ADIOSInterfaceWriteTest.DefineVar_int8_t_1x10
     1/62 Test  #1: ADIOSInterfaceWriteTest.DefineVar_int8_t_1x10 ............................   Passed    0.16 sec
          Start  2: ADIOSInterfaceWriteTest.DefineVar_int16_t_1x10
     2/62 Test  #2: ADIOSInterfaceWriteTest.DefineVar_int16_t_1x10 ...........................   Passed    0.06 sec
          Start  3: ADIOSInterfaceWriteTest.DefineVar_int32_t_1x10
          
          ...
               
          Start 61: ADIOSBZip2Wrapper.WrongParameterValue
    61/62 Test #61: ADIOSBZip2Wrapper.WrongParameterValue ....................................   Passed    0.00 sec
          Start 62: ADIOSBZip2Wrapper.WrongBZip2Name
    62/62 Test #62: ADIOSBZip2Wrapper.WrongBZip2Name .........................................   Passed    0.00 sec
    
    100% tests passed, 0 tests failed out of 62
    
    Total Test time (real) =   3.95 sec
    

5. Install  

.. code-block:: bash 
    
    $ make install


*******************************
Build CMake -DVAR=VALUE Options
*******************************

The following options can be specified with CMake's `-DVAR=VALUE` syntax to control which features get enabled or disabled, default option (ON/OFF) is highlighted:

====================== ========================= ==========================================================================================================================================================================================================================
CMake VAR Option        Values                    Description                                                                     
====================== ========================= ==========================================================================================================================================================================================================================
 `ADIOS2_USE_MPI`      **`AUTO`**/``ON``/OFF      Enable MPI or non-MPI (serial) build.                                                                      
 `ADIOS2_USE_DataMan`  **`AUTO`**/``ON``/OFF      Enable the DataMan engine for Wide-Area-Network transports.                                    
 `ADIOS2_USE_ZeroMQ`   **`AUTO`**/``ON``/OFF      Enable `ZeroMQ <http://zeromq.org/>`_ for the DataMan engine.                                            
 `ADIOS2_USE_HDF5`     **`AUTO`**/``ON``/OFF      Enable the `HDF5 <https://www.hdfgroup.org>`_ engine. If HDF5 is not in the path or not the correct version is in the path, set the correct path by the -DHDF5_ROOT=... option      
 `ADIOS2_USE_ADIOS1`   **`AUTO`**/``ON``/OFF      Enable the `ADIOS 1.x <https://www.olcf.ornl.gov/center-projects/adios>`_ engine, only v1.12 or above are valid.   
 `ADIOS2_USE_Python`   **`AUTO`**/``ON``/OFF      Enable the Python >= 2.7 bindings. mpi4py and numpy. Python 3 will be used if Python 2 and 3 are found. If you want a python version not in the path then choose the right pyhton executable by -DPYTHON_EXECUTABLE=... 
 `ADIOS2_USE_Fortran`  **`AUTO`**/ON/``OFF``      Enable the Fortran 90 or above bindings. Must have a Fortran compiler. Default is OFF, must be explicitly set to ON.
 `ADIOS2_USE_BZip2`    **`AUTO`**/``ON``/OFF      Enable `BZip2 <http://www.bzip.org>`_ compression (experimental, not yet implemented).              
 `ADIOS2_USE_ZFP`      **`AUTO`**/``ON``/OFF      Enable `ZFP <https://github.com/LLNL/zfp>`_ compression (experimental, not yet implemented).  
====================== ========================= ==========================================================================================================================================================================================================================

Examples: Enable Fortran, disable Python bindings and ZeroMQ functionality 

.. code-block:: bash

    $ cmake -DADIOS2_USE_Fortran=ON -DADIOS2_USE_Python=OFF -DADIOS2_USE_ZeroMQ=OFF ../ADIOS2


Notes: 
   * The `ADIOS2_USE_HDF5` and `ADIOS2_USE_ADIOS1` options require the use of a matching serial or parallel version depending on whether `ADIOS2_USE_MPI` is enabled. Similary, enabling MPI and Python bindings require `mpi4py`.
   
   * Optional ROOT suffix to a dependency can guide cmake into finding a particular dependency:
   
.. code-block:: bash

    $ cmake -DADIOS1_ROOT=/opt/adios/1.12.0 ../ADIOS2

In addition to the `ADIOS2_USE_Feature` options, the following options are also available to control how the library gets built:

============================ =================================================== ============================================
CMake VAR Options              Values                                              Description                                                                           |
============================ =================================================== ============================================
 `BUILD_SHARED_LIBS`          ``ON``/OFF                                          Build shared libraries.                                                               
 `ADIOS2_ENABLE_PIC`          ``ON``/OFF                                          Enable Position Independent Code.                                
 `ADIOS2_BUILD_EXAMPLES`      ``ON``/OFF                                          Build examples.                                                                       
 `ADIOS2_BUILD_TESTING`       ``ON``/OFF                                          Build test code.                                                                      
 `CMAKE_INSTALL_PREFIX`       /path/to/install (``/usr/local``)                   Install location.                                                                     
 `CMAKE_BUILD_TYPE`           ``Debug`` / Release / RelWithDebInfo / MinSizeRel   The level of compiler optimization to use.                                            
============================ =================================================== ============================================

Example: the following configuration will build, test and install under /opt/adios2/2.2.0 an optimized (Release) version of ADIOS2.

.. code-block:: bash

    $ cd build 
    $ cmake -DADIOS2_USE_Fortran=ON -DCMAKE_INSTALL_PREFIX=/opt/adios2/2.2.0 -DCMAKE_BUILD_Type=Release ../ADIOS2
    $ make -j 4
    $ ctest
    $ make install