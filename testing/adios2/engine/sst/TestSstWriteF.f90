program TestSstWrite
  use sst_test_data
  use mpi
  use adios2
  implicit none

  integer(kind = 8), dimension(1)::shape_dims, start_dims, count_dims
  integer(kind = 8), dimension(2)::shape_dims2, start_dims2, count_dims2
  integer::inx, irank, isize, ierr, i, insteps

  type(adios2_adios)::adios
  type(adios2_io)::ioWrite, ioRead
  type(adios2_variable), dimension(12)::variables
  type(adios2_engine)::sstWriter;

  !read handlers
  character(len =:), allocatable::variable_name 
  integer::variable_type, ndims
  integer(kind = 8), dimension(:), allocatable::shape_in
  
  !Launch MPI
  call MPI_Init(ierr) 
  call MPI_Comm_rank(MPI_COMM_WORLD, irank, ierr) 
  call MPI_Comm_size(MPI_COMM_WORLD, isize, ierr)

  !Application variables 
  inx = 8

  insteps = 1;

  !Variable dimensions 
  shape_dims(1) = isize * inx
  start_dims(1) = irank * inx
  count_dims(1) = inx

  shape_dims2 = (/ 2, isize *inx /)
  start_dims2 = (/ 0, irank *inx /)
  count_dims2 = (/ 2, inx /)

  !Create adios handler passing the communicator, debug mode and error flag
  call adios2_init(adios, MPI_COMM_WORLD, adios2_debug_mode_on, ierr)

!!!!!!!!!!!!!!!!!!!!!!!!!WRITER!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!!!!!!!!Declare an IO process configuration inside adios 
  call adios2_declare_io(ioWrite, adios, "ioWrite", ierr)

  call adios2_set_engine(ioWrite, "Sst", ierr)

  !Defines a variable to be written 
  call adios2_define_variable(variables(1), ioWrite, "i8", &
       adios2_type_integer1, 1, &
       shape_dims, start_dims, count_dims, &
       adios2_constant_dims, ierr)
  
  call adios2_define_variable(variables(2), ioWrite, "i16", &
       adios2_type_integer2, 1, &
       shape_dims, start_dims, count_dims, &
       adios2_constant_dims, ierr)

  call adios2_define_variable(variables(3), ioWrite, "i32", &
       adios2_type_integer4, 1, &
       shape_dims, start_dims, count_dims, &
       adios2_constant_dims, ierr)

  call adios2_define_variable(variables(4), ioWrite, "i64", &
       adios2_type_integer8, 1, &
       shape_dims, start_dims, count_dims, &
       adios2_constant_dims, ierr)

  call adios2_define_variable(variables(5), ioWrite, "r32", &
       adios2_type_real, 1, &
       shape_dims, start_dims, count_dims,&
       adios2_constant_dims, ierr)

  call adios2_define_variable(variables(6), ioWrite, "r64", &
       adios2_type_dp, 1, &
       shape_dims, start_dims, count_dims, &
       adios2_constant_dims, ierr)

  call adios2_define_variable(variables(7), ioWrite, "r64_2d", &
       adios2_type_dp, 2, &
       shape_dims2, start_dims2, count_dims2, &
       adios2_constant_dims, ierr)

  call adios2_open(sstWriter, ioWrite, "ADIOS2Sst", adios2_mode_write, ierr)

  !Put array contents to bp buffer, based on var1 metadata
  do i = 1, insteps
     call GenerateTestData(i - 1, irank, isize)
     call adios2_begin_step(sstWriter, adios2_step_mode_append, 0.0, ierr)
     call adios2_put(sstWriter, variables(1), data_I8, ierr)
     call adios2_put(sstWriter, variables(2), data_I16, ierr)
     call adios2_put(sstWriter, variables(3), data_I32, ierr)
     call adios2_put(sstWriter, variables(4), data_I64, ierr)
     call adios2_put(sstWriter, variables(5), data_R32, ierr)
     call adios2_put(sstWriter, variables(6), data_R64, ierr)
     print *, "Data_r64_2d ", data_R64_2d(1, 1)
     call adios2_put(sstWriter, variables(7), data_R64_2d, ierr)
     call adios2_end_step(sstWriter, ierr)
  end do

  !Closes engine1 and deallocates it, becomes unreachable
  call adios2_close(sstWriter, ierr)

   !Deallocates adios and calls its destructor 
   call adios2_finalize(adios, ierr)

   call MPI_Finalize(ierr)

 end program TestSstWrite
