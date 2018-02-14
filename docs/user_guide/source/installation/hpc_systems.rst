****************************************************
Building on High Performance Computing, HPC, Systems
****************************************************

Please read this section before building ADIOS2 with CMake on a Supercomputer:  

1. **Modules with minimum requirements:** make sure the corresponding "module" dependencies are loaded and that minimum requirements are satisfied. Load the latest cmake module as many systems have older cmake versions as default. The same with your build environment, make sure compilers support C++11. This includes gcc >= 4.8.1, Intel >= 15, PGI >= 15, etc.

2. **Optimization flags:** for actual performance tests and operations, it is recommended to set -DCMAKE_BUILD_TYPE=Release explicitly so CMake can turn on appropriate optimization flags. 

3. **Static/Dynamic build:** On most Cray systems, like `Titan <https://www.olcf.ornl.gov/kb_articles/compiling-and-node-types/>`_, the default library link behavior is "static", thus CMake builds ADIOS2 statically (libadios2.a) by default. Read the system's documentation to enable dynamic compilation, usually by setting an environment variable such as `CRAYPE_LINK_TYPE=dynamic`.

4. **Big Endian and 32-bit systems:** ADIOS2 hasn't been tested on big endian or 32-bit systems. Please be aware before attempting to run.

5. **System Guidelines and Policies:** follow your system guidelines and policies for launching jobs, executing MPI communication, memory allocation, and I/O storage before attempting any type of unconventional usage of ADIOS2.
