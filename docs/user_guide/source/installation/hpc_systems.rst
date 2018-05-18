****************************************************
Building on High Performance Computing, HPC, Systems
****************************************************

Please read this section before building ADIOS2 with CMake on a Supercomputer:  

#. **Modules with minimum requirements:** Make sure the corresponding "module" dependencies are loaded and that minimum requirements are satisfied. Load the latest cmake module as many systems have older cmake versions as default. The same with your build environment, make sure compilers support C++11. This includes gcc >= 4.8.1, Intel >= 15, PGI >= 15, etc.

#. **Optimization flags:** For actual performance tests and operations, it is recommended to set -DCMAKE_BUILD_TYPE=Release explicitly so CMake can turn on appropriate optimization flags. 

#. **Static/Dynamic build:** On most Cray systems, like `Titan <https://www.olcf.ornl.gov/kb_articles/compiling-and-node-types/>`_, the default library link behavior is "static", thus CMake builds ADIOS2 statically (libadios2.a) by default. Read the system's documentation to enable dynamic compilation, usually by setting an environment variable such as `CRAYPE_LINK_TYPE=dynamic`.

#. **Big Endian and 32-bit systems:** ADIOS2 hasn't been tested on big endian or 32-bit systems. Please be aware before attempting to run.

#. **System guidelines and policies:** Follow your system guidelines and policies for launching jobs, executing MPI communication, memory allocation, and I/O storage before attempting any type of unconventional usage of ADIOS2.

#. **CMake minimum version:** The ADIOS2 build system requires a minimum CMake version of 3.6.0, however, if using IBM XL, Cray, or PGI compilers, 3.9.0 or newer is required.  ADIOS2 is using CMake's language level abstraction to instruct the compiler to build with C++11 support.  This feature was available in CMake all the way back to 3.3, but with the feature only supporting GCC, Clang, and MSVC compilers.  Langiuage level abstraction support was added for Intel compilers in CMake 3.6.0 and for all other compilers supproted by CMake (including XL, Cray, and PGI) in 3.9.0.

#. **PGI compilers and C++11 support:** Recent versions of the PGI compiler >= v15 support the C++11 language standard, however they rely on the set of C++ standard library headers supplied by GCC.  On most systems, the default configuration at the time compiler is instaled is to use the headers supplied by the system gcc in /usr.  On many systems (Titan at OLCF, for example) even though the PGI compiler supports C++11 the configured GCC and it's headers do not (4.3.x on Cray Linux Environment v5 systems like Titan).  To configure the PGI compiler to use a newer GCC, you will need to create a configuration file in your home directory that overrides the PGI compiler's default configuration.  On Titan, the following steps will re-configure the PGI compiler to use GCC 6.3.0 provided by a module:

.. code-block:: bash

  $ module load gcc/6.3.0
  $ makelocalrc $(dirname $(which pgc++)) -gcc $(which gcc) -gpp $(which g++) -g77 $(which gfortran) -o -net 1>${HOME}/.mypgirc 2>/dev/null

