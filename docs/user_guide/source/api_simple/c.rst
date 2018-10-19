*****************
C simple bindings
*****************

Write example:

.. code-block:: c

   #include <adios2_c.h>
   ...
   
   int rank, size;
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   MPI_Comm_size(MPI_COMM_WORLD, &size);
   
   // Nx, Ny from application, size_t
   size_t shape[2];
   shape[0] = Nx;
   shape[1] = Ny * (size_t) size;
   
   size_t start[2];
   start[0] = 0;
   start[1] = Ny * (size_t) rank;
   
   size_t count[2];
   start[0] = Nx;
   start[1] = Ny;
   
   adios2_FILE* fh = adios2_fopen( "cfd.bp", "w", MPI_COMM_WORLD);
   adios2_error errio; //optionally check error return type
   // NSteps from aplication
   for (size_t step = 0; step < NSteps; ++step)
   {
       if(rank == 0 && step == 0) // global variable
       {
           //single value syntax
           errio = adios2_fwrite(fh, "varR32", adios2_type_int, &size, 0, 
                                 NULL, NULL, NULL, adios2_false);
       }

       // physicalTime value
       errio = adios2_fwrite(fh, "physicalTime", adios2_type_double,  
                             &physical_time,   
                             0, NULL, NULL, NULL, adios2_false);
       // temperature and pressure arrays T and P
       errio = adios2_fwrite(fh, "temperature", adios2_type_float,  
                             T, 
                             2, shape, start, count, adios2_false);
       /// advance to next step by passing adios2_true
       errio = adios2_fwrite(fh, "pressure", adios2_type_float,
                             P,    
                             2, shape, start, count, adios2_true);
   }
   
   // mandatory, otherwise resource and memory leaks happen 
   adios2_fclose(fh); 

Read "stepping/streaming" example:

.. code-block:: c++
   
   #include <adios2_c.h>
   ...
   
   int rank, size;
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   MPI_Comm_size(MPI_COMM_WORLD, &size);
   
   // Selection Window from application, std::size_t
   if( rank == 0)
   {
      size_t start[2];
      start[0] = 0;
      start[1] = 0;
   
      size_t count[2];
      start[0] = selX;
      start[1] = selY;
   
      // if only one rank is active use MPI_COMM_SELF
      adios2_FILE* fh = adios2_fopen( "cfd.bp", "r", MPI_COMM_WORLD);
      adios2_error errio; //optionally check error return type
      
      adios2_step* fsh;
      
      while( adios2_fgets(fsh, fh) != NULL )
      {
          // read value signature
          errio = adios2_fread(fsh, "physicalTime", adios2_type_double, 
                                &physical_time, 0, NULL, NULL); 
          
          // T and P must be pre-allocated
          errio = adios2_fread(fsh, "temperature", adios2_type_float, 
                                T, 2, start, count);
          
          errio = adios2_fread(fsh, "temperature", adios2_type_float, 
                                P, 2, start, count);
          
          // use T and P for current step
      }
      // mandatory, otherwise resource and memory leaks happen 
      adios2_fclose(fh); 
   }



``adios2_FILE*`` API documentation

.. doxygenfile:: adios2_c_FILE.h
   :project: C
   :path: ../../bindings/C/c/
   
   
  