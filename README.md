# ADIOSPP

Next generation ADIOS in C++ for exascale computations 
Read ./doc/CodingGuidelines first

Create ./lib and ./bin libraries before compiling 
make      -> build ./lib/libadios.a with MPI and noMPI code
make mpi  -> build ./lib/libadios.a with MPI code only 
make nompi  -> build ./lib/libadios_nompi.a with serial (noMPI) code only


