# ADIOSPP

Next generation ADIOS in C++11/14 for exascale computations. 
Read ./doc/CodingGuidelines first if you are a developer.
Doxygen documentation can be generated running doxygen under ./doc, a ./doc/html directory will be created

Requirements: 
1) C++11 compiler (e.g. gnu gcc 4.8.x and above) in PATH, default is g++
2) MPI compiler (e.g. openmpi, mpich2 ) in PATH, default is mpic++
 
make      -> build ./lib/libadios.a and ./lib/libadios_nompi.a
make mpi  -> build ./lib/libadios.a with truly MPI code (from mpi.h) using mpic++
make nompi -> build ./lib/libadios_nompi.a with serial (dummy MPI) code only calling mpiduumy.h using g++ (C++11)

For examples, start with examples/hello/writer/helloWriter_OOP.cpp, build as above after ADIOS library is built 