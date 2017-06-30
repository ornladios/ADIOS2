examples/heatTransfer

Test that solves a 2D Poisson equation for temperature in homogeneous media
using finite differences. This examples shows a straight-forward way to hook an
application to the ADIOS2 library for its IO.


1. read: illustrates an experimental Read API, uses adios1 underneath  

2. write: illustrates the Write API, resulting binaries under build/bin require arguments  
  
    * adios1    
    * adios2
    * hdf5
    * phdf5