##########################
Frequently Asked Questions
##########################


MPI vs Non-MPI
**************

#. :ref:`Can I use the same library for MPI and non-MPI code?`

APIs
****

#. :ref:`Can I use ADIOS 2 C++11 library with C++98 codes?`
#. :ref:`Why are C and Fortran APIs missing functionality?`
#. :ref:`C++11: Why are std::string arguments passed sometimes by value and sometimes by reference?`
#. :ref:`C++11: Should I pass adios2:: objects by value or by reference?`
#. :ref:`Fortran: Can I pass slices and temporary arrays to adios2_put?`

Building on Titan
*****************

#. :ref:`My application uses PGI compilers on Titan, can I link ADIOS 2?`
#. :ref:`How do I enable the Python bindings on Titan?`

Building and Running on Fujitsu FX100
*************************************

#. :ref:`How do I build ADIOS 2 on Fujitsu FX100?`
#. :ref:`SST engine hangs on Fujitsu FX100. Why?`

FAQs Answered
*************

Can I use the same library for MPI and non-MPI code?
----------------------------------------------------

Short answer: No.

Long answer: This created conflicts in the past, as the MPI APIs were mocked in the sequential version.
If you need "sequential" behavior with the MPI library, use ``MPI_COMM_SELF``.
Always pass a communicator in the MPI version


Can I use ADIOS 2 C++11 library with C++98 codes?
-------------------------------------------------

Use the :ref:`C bindings`. C++11 is a brand new language standard and many new (and old, *e.g.* ``std::string``) might cause ABI conflicts.

Why are C and Fortran APIs missing functionality?
-------------------------------------------------

Because language instrinsics are NOT THE SAME. For example, C++ and Python support key/value pair structures natively, *e.g.* ``std::map`` and dictionaries, respectively.
Fortran and C only support arrays natively.
Use the right language (tool) for the right task.


C++11: Why are ``std::string`` arguments passed sometimes by value and sometimes by reference?
----------------------------------------------------------------------------------------------

C++11, provides mechanisms to optimize copying small objects, rather than passing by reference. The latter was always the rule for C++98. When a string is passed by value, it's assumed that the name will be short, <= 15 characters, most of the time. While passing by reference indicates that the string can be of any size. Check the `isocpp guidelines on this topic <http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#f15-prefer-simple-and-conventional-ways-of-passing-information>`_ for more information.


C++11: Should I pass adios2:: objects by value or by reference?
---------------------------------------------------------------

``adios2::ADIOS``: always pass by reference this is the only "large memory" object; all others: pass by reference or value depending on your coding standards and requirements, they are small objects that wrap around a pointer to an internal object inside ``adios2::ADIOS``.


Fortran: Can I pass slices and temporary arrays to ``adios2_put``?
------------------------------------------------------------------

By definition the lifetime of a temporary if the scope of the function is passed to. Therefore,
you must use sync mode with ``adios2_put``.
Deferred mode will save garbage data since the memory location of a temporary is undefined after ``adios2_put``, not able to reach ``adios2_end_step``, ``adios2_close`` or ``adios2_perform_puts`` where the memory is actually used.



My application uses PGI compilers on Titan, can I link ADIOS 2?
---------------------------------------------------------------

Follow directions at :ref:`Building on High Performance Computing, HPC, Systems` to setup support for PGI on Titan. PGI compilers depend on GNU headers, but they must point to a version greater than gcc 4.8.1 to support C++11 features. The gcc module doesn't need to be loaded, though. Example:

   .. code-block:: bash

      $ module load gcc/7.2.0
      $ makelocalrc $(dirname $(which pgc++)) -gcc $(which gcc) -gpp $(which g++) -g77 $(which gfortran) -o -net 1>${HOME}/.mypgirc 2>/dev/null
      $ module unload gcc/7.2.0


How do I enable the Python bindings on Titan?
---------------------------------------------

ADIOS 2 default configuration on Titan is to build the static library. Python bindings require enabling the dynamic libraries and the Cray dynamic environment variable. See :ref:`Building on High Performance Computing, HPC, Systems` and  :ref:`Enabling the Python bindings`. For example:

   .. code-block:: bash

      [atkins3@titan-ext4 code]$ mkdir adios
      [atkins3@titan-ext4 code]$ cd adios
      [atkins3@titan-ext4 adios]$ git clone https://github.com/ornladios/adios2.git source
      [atkins3@titan-ext4 adios]$ module swap PrgEnv-pgi PrgEnv-gnu
      [atkins3@titan-ext4 adios]$ module load cmake3/3.11.3
      [atkins3@titan-ext4 adios]$ module load python python_numpy python_mpi4py
      [atkins3@titan-ext4 adios]$ export CRAYPE_LINK_TYPE=dynamic CC=cc CXX=CC FC=ftn
      [atkins3@titan-ext4 adios]$ mkdir build
      [atkins3@titan-ext4 build]$ cd build
      [atkins3@titan-ext4 build]$ cmake ../source
      -- The C compiler identification is GNU 6.3.0
      -- The CXX compiler identification is GNU 6.3.0
      -- Cray Programming Environment 2.5.13 C
      -- Check for working C compiler: /opt/cray/craype/2.5.13/bin/cc
      -- Check for working C compiler: /opt/cray/craype/2.5.13/bin/cc -- works
      -- Detecting C compiler ABI info
      -- Detecting C compiler ABI info - done
      -- Detecting C compile features
      -- Detecting C compile features - done
      -- Cray Programming Environment 2.5.13 CXX
      -- Check for working CXX compiler: /opt/cray/craype/2.5.13/bin/CC
      -- Check for working CXX compiler: /opt/cray/craype/2.5.13/bin/CC -- works
      ...
      -- Found PythonInterp: /sw/titan/.swci/0-login/opt/spack/20180315/linux-suse_linux11-x86_64/gcc-4.3.4/python-2.7.9-v6ctjewwdx6k2qs7ublexz7gnx457jo5/bin/python2.7 (found version "2.7.9") 
      -- Found PythonLibs: /sw/titan/.swci/0-login/opt/spack/20180315/linux-suse_linux11-x86_64/gcc-4.3.4/python-2.7.9-v6ctjewwdx6k2qs7ublexz7gnx457jo5/lib/libpython2.7.so (found version "2.7.9") 
      -- Found PythonModule_numpy: /sw/xk6/python_numpy/1.7.1/python2.7.9_craylibsci_gnu4.9.0/lib64/python2.7/site-packages/numpy  
      -- Found PythonModule_mpi4py: /lustre/atlas/sw/xk7/python_mpi4py/2.0.0/cle5.2up04_python2.7.9/lib64/python2.7/site-packages/mpi4py  
      -- Found PythonFull: /sw/titan/.swci/0-login/opt/spack/20180315/linux-suse_linux11-x86_64/gcc-4.3.4/python-2.7.9-v6ctjewwdx6k2qs7ublexz7gnx457jo5/bin/python2.7  found components:  Interp Libs numpy mpi4py 
      ...
      ADIOS2 build configuration:
        ADIOS Version: 2.3.1
        C++ Compiler : GNU 6.3.0 CrayPrgEnv
          /opt/cray/craype/2.5.13/bin/CC

        Fortran Compiler : GNU 6.3.0 CrayPrgEnv
          /opt/cray/craype/2.5.13/bin/ftn

        Installation prefix: /usr/local
              bin: bin
              lib: lib
          include: include
            cmake: lib/cmake/adios2
           python: lib/python2.7/site-packages

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
            ZeroMQ   : OFF
            HDF5     : OFF
            Python   : ON
            Fortran  : ON
            SysVShMem: ON
            Endian_Reverse: OFF

      -- Configuring done
      -- Generating done
      -- Build files have been written to: /ccs/home/atkins3/code/adios/build


How do I build ADIOS 2 on Fujitsu FX100?
----------------------------------------

* Cross-compilation (building on the login node) is not recommended. Submit an
  interactive job and build on the compute nodes.
* Make sure CMake >= 3.6 is installed on the compute nodes. If not, you need
  to build and install it from source since CMake does not provide SPARC V9
  binaries.
* Use gcc instead of the Fujitsu compiler. We tested with gcc 6.3.0
* CMake fails to automatically find the correct MPI library on FX100. As a
  workaround, set CC, CXX, and FC to the corresponding MPI compiler wrappers:

   .. code-block:: bash

      $ CC=mpigcc CXX=mpig++ FC=mpigfortran cmake  ..

SST engine hangs on Fujitsu FX100. Why?
---------------------------------------

The communication thread of SST might have failed to start. FX100 requires
users to set the maximum stack size manually when launching POSIX threads.
One way to do this is through ulimit (*e.g.* ``ulimit -s 1024``). You can
also set the stack size when submitting the job. Please contact your system
administrator for details.
