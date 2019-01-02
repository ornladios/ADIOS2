############
Using ADIOS in your own projects
############

ADIOS2 uses `CMake <https://cmake.org/>`_ version 3.6 or above.  You consume it directly from another CMake project, use the `find_package` call and set the `ADIOS2_ROOT` or `ADISO2_DIR` environment variables to the install prefix for adios:

.. code-block:: cmake
    cmake_minimum_required(VERSION 3.6)
    project(MySimulation C CXX)

    find_package(MPI REQUIRED)
    find_package(ADIOS2 REQUIRED)

    ...
    add_library(my_library src1.cxx src2.cxx...)
    target_link_libraries(my_library PRIVATE adios2::adios2 MPI::MPI_C ...)


Or if you're not using CMake then you can manually get the necessary compile and link flags for your project using the adios2-config tool from the installation folder:

.. code-block:: bash
    $ /path/to/install-prefix/bin/adios2-config --cxxflags
    ADIOS2_DIR: /home/khq.kitware.com/chuck.atkins/Code/adios2/install/master
-isystem /home/khq.kitware.com/chuck.atkins/Code/adios2/install/master/include -isystem /opt/ohpc/pub/mpi/openmpi3-gnu7/3.1.0/include -pthread -std=gnu++11
    $ /path/to/install-prefix/bin/adios2-config --cxxlibs
    ADIOS2_DIR: /home/khq.kitware.com/chuck.atkins/Code/adios2/install/master
-Wl,-rpath,/home/khq.kitware.com/chuck.atkins/Code/adios2/install/master/lib:/opt/ohpc/pub/mpi/openmpi3-gnu7/3.1.0/lib /home/khq.kitware.com/chuck.atkins/Code/adios2/install/master/lib/libadios2.so.2.3.0 -pthread -Wl,-rpath -Wl,/opt/ohpc/pub/mpi/openmpi3-gnu7/3.1.0/lib -Wl,--enable-new-dtags -pthread /opt/ohpc/pub/mpi/openmpi3-gnu7/3.1.0/lib/libmpi.so -Wl,-rpath-link,/home/khq.kitware.com/chuck.atkins/Code/adios2/install/master/lib

