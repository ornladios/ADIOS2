################################
Using ADIOS in your own projects
################################

**********
From CMake
**********

ADIOS exports a CMake package configuration file that allows it's targets to be directly imported into another CMake project via CMake's ``find_package`` command:

.. code-block:: cmake

    cmake_minimum_required(VERSION 3.6)
    project(MySimulation C CXX)

    find_package(MPI REQUIRED)
    find_package(ADIOS2 REQUIRED)
    #...
    add_library(my_library src1.cxx src2.cxx...)
    target_link_libraries(my_library PRIVATE adios2::adios2 MPI::MPI_C ...)

When configuring your project you can then set the ``ADIOS2_ROOT`` or ``ADIOS2_DIR`` environment variables to the install prefix for adios.  You can also point it to an ADIOS build directory, which may be easier when using development versions of ADIOS than you may not want installed system-wide

****************************
From non-CMake build systems
****************************

If you're not using CMake then you can manually get the necessary compile and link flags for your project using the ``adios2-config`` tool from the installation folder:

.. code-block:: bash

    $ /path/to/install-prefix/bin/adios2-config --cxxflags
    ADIOS2_DIR: /path/to/install-prefix
    -isystem /path/to/install-prefix/include -isystem /opt/ohpc/pub/mpi/openmpi3-gnu7/3.1.0/include -pthread -std=gnu++11
    $ /path/to/install-prefix/bin/adios2-config --cxxlibs
    ADIOS2_DIR: /path/to/install-prefix
    -Wl,-rpath,/path/to/install-prefix/lib:/opt/ohpc/pub/mpi/openmpi3-gnu7/3.1.0/lib /path/to/install-prefix/lib/libadios2.so.2.3.1 -pthread -Wl,-rpath -Wl,/opt/ohpc/pub/mpi/openmpi3-gnu7/3.1.0/lib -Wl,--enable-new-dtags -pthread /opt/ohpc/pub/mpi/openmpi3-gnu7/3.1.0/lib/libmpi.so -Wl,-rpath-link,/path/to/install-prefix/lib

