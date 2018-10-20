***********************
Fortran simple bindings
***********************

The Fortran simple API bindings are based on the C adios2_FILE* interface and the native Fortran IO ``advance`` parameter for stepping at write and read. They consists only on subroutines and a single ``type(adios2_file)`` handler for adios2 streams.

On the read side, allocatable arrays must be passed without pre-allocation. ADIOS2 allocates the array memory based on the read selection dimensions and steps. 


.. note::

   ADIOS2 supports Fortran 90 and more recent standards only.


Write example:

.. code-block:: fortran

   import adios2
   ...

   type(adios2_file) :: fh
   integer(kind=8), dimension(2):: shape_dims, start_dims, count_dims
   integer:: rank, size 
   ...
   
   call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
   call MPI_Comm_size(MPI_COMM_WORLD, size, ierr)
   
   ! Nx, Ny from application, integer
   shape_dims(1) = Nx
   shape_dims(2) = Ny * size
   
   start_dims(1) = 0
   start_dims(2) = Ny * rank
   
   count_dims(1) = Nx
   count_dims(2) = Ny

   call adios2_fopen(fh, 'cfd.bp', 'w', MPI_COMM_WORLD, ierr)
   
   ! NSteps from aplication
   do i = 1, NSteps
      ! write global value
      if( rank == 0 .and. step == 1 ) then
          call adios2_fwrite(fh, "mpi_size", adios2_type_integer4, size, ierr)
      end if
      
      ! physicalTime value
      call adios2_fwrite(fh, "physicalTime", adios2_type_dp, & 
                         physical_time, ierr)  
      ! temperature and pressure arrays
      call adios2_fwrite(fh, "temperature", adios2_type_float, & 
                         temperature, shape, start, count, ierr)
      ! advance to next step after writing pressure
      call adios2_fwrite(fh, "pressure", adios2_type_float, & 
                         pressure, shape, start, count, adios2_advance_yes, ierr)
   end do
   ! mandatory, otherwise resource and memory leaks happen 
   call adios2_fclose(fh, ierr); 

Read "stepping/streaming" example:

.. code-block:: fortran

   import adios2
   ...

   type(adios2_file) :: fh
   integer(kind=8), dimension(2):: start_dims, count_dims
   integer:: rank, size 
   integer(kind=4):: size_in
   real(kind=8):: physical_time
   real, dimension(:,:), allocatable:: temperature, pressure
   ...
   
   call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
   call MPI_Comm_size(MPI_COMM_WORLD, size, ierr)
   
   ! Nx, Ny from application, integer
   if( rank == 0 ) then 
   
      start_dims(1) = 0
      start_dims(2) = 0
   
      count_dims(1) = selX
      count_dims(2) = selY
      
      ! if only one rank is active use MPI_COMM_SELF 
      call adios2_fopen(fh, 'cfd.bp', 'w', MPI_COMM_SELF, ierr)
      
      i = 0
      
      do
      
         if( i == 0) then
             call adios2_fread(fh, "mpi_size", size_in, ierr)
         end if
         
         call adios2_fread(fh, "physical_time", physical_time, ierr)
         call adios2_fread(fh, "temperature", temperature, &
                           start_dims, count_dims, ierr)
         call adios2_fread(fh, "temperature", temperature, &
                          start_dims, count_dims, adios2_advance_yes, ierr)
         ! exit do loop
         if (ierr < 0) exit
         i = i + 1
      end do
      
      ! mandatory, otherwise resource and memory leaks happen
      call adios2_fclose(fh, ierr) 