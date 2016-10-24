# ADIOSPP

Next generation ADIOS in C++ for exascale computations 
Read ./doc/CodingGuidelines first

Create ./lib and ./bin libraries before compiling 
make      -> build ./lib/libadios.a with truly MPI code (from mpi.h)
make mpi  -> build ./lib/libadios.a with truly MPI code (from mpi.h)
make nompi  -> build ./lib/libadios_nompi.a with serial (dummy MPI) code only calling ./public/mpiduumy.h


