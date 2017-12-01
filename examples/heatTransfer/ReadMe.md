examples/heatTransfer

Test that solves a 2D Poisson equation for temperature in homogeneous media
using finite differences. This examples shows a straight-forward way to hook 
an application to the ADIOS2 library for its IO.


1. read: illustrates the Read API that allows running the reader either as

    * post-mortem to read all output steps
    * in situ to read step by step as the writer outputs them

2. write: illustrates the Write API as well as has implementations of other IO libraries
  
    * adios 1.x    
    * hdf5
    * phdf5

3. read_fileonly: illustrates reading all output steps at once (a single read 
   statement) into a single contiguous memory block. This approach only works 
   for post-mortem processing. 
