subroutine usage()
    print *, "Usage: TestCommonWriteF engine filename"
end subroutine usage


program TestSstWrite
  use sst_test_data
#ifdef ADIOS2_HAVE_MPI_F
  use mpi
#endif
  use adios2
  implicit none

#ifndef __GFORTRAN__
#ifndef __GNUC__
  interface
     integer function iargc()
     end function iargc
  end interface
#endif
#endif

  integer :: numargs

  integer(kind = 8), dimension(1)::shape_dims, start_dims, count_dims
  integer(kind = 8), dimension(2)::shape_dims2, start_dims2, count_dims2
  integer(kind = 8), dimension(2)::shape_dims3, start_dims3, count_dims3
  integer(kind = 8), dimension(1)::shape_time, start_time, count_time
  integer::inx, irank, isize, ierr, i, insteps, status

  character(len=256) :: filename, engine

  type(adios2_adios)::adios
  type(adios2_io)::ioWrite, ioRead
  type(adios2_variable), dimension(20)::variables
  type(adios2_engine)::sstWriter;

  !read handlers
  character(len =:), allocatable::variable_name 
  integer::variable_type, ndims
  integer(kind = 8), dimension(:), allocatable::shape_in
  integer(kind = 8)::localtime
  integer::color, key, testComm

  numargs = iargc()
  if ( numargs < 2 ) then
     call usage()
     call exit(1)
  endif


  call getarg(1, engine)
  call getarg(2, filename)

#ifdef ADIOS2_HAVE_MPI_F
  !Launch MPI
  call MPI_Init(ierr) 

  call MPI_Comm_rank(MPI_COMM_WORLD, key, ierr);

  color = 1
  call MPI_Comm_split(MPI_COMM_WORLD, color, key, testComm, ierr);

  call MPI_Comm_rank(testComm, irank, ierr) 
  call MPI_Comm_size(testComm, isize, ierr)
#else
  ! No MPI
  irank = 0;
  isize = 1;
#endif

  !Application variables 
  insteps = 10;

  !Variable dimensions 
  shape_dims(1) = isize * nx
  start_dims(1) = irank * nx
  count_dims(1) = nx

  shape_dims2 = (/ 2, isize *nx /)
  start_dims2 = (/ 0, irank *nx /)
  count_dims2 = (/ 2, nx /)

  shape_dims3 = (/ isize *nx, 2 /)
  start_dims3 = (/ irank *nx, 0 /)
  count_dims3 = (/ nx, 2 /)

  shape_time = (/ isize /)
  start_time = (/ irank /)
  count_time = (/ 1 /)

#ifdef ADIOS2_HAVE_MPI_F
  !Create adios handler passing the communicator, debug mode and error flag
  call adios2_init(adios, testComm, adios2_debug_mode_on, ierr)
#else
  call adios2_init_nompi(adios, adios2_debug_mode_on, ierr)
#endif

!!!!!!!!!!!!!!!!!!!!!!!!!WRITER!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!!!!!!!!Declare an IO process configuration inside adios 
  call adios2_declare_io(ioWrite, adios, "ioWrite", ierr)

  call adios2_set_engine(ioWrite, engine, ierr)

  !Defines a variable to be written 
  call adios2_define_variable(variables(12), ioWrite, "scalar_r64", &
       adios2_type_dp, ierr)
  
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

  call adios2_define_variable(variables(8), ioWrite, "r64_2d_rev", &
       adios2_type_dp, 2, &
       shape_dims3, start_dims3, count_dims3, &
       adios2_constant_dims, ierr)

  call adios2_define_variable(variables(9), ioWrite, "time", &
       adios2_type_integer8, 1, &
       shape_time, start_time, count_time, &
       adios2_constant_dims, ierr)

  call adios2_define_variable(variables(10), ioWrite, "c32", &
       adios2_type_complex, 1, &
       shape_dims, start_dims, count_dims,&
       adios2_constant_dims, ierr)

  call adios2_define_variable(variables(11), ioWrite, "c64", &
       adios2_type_complex_dp, 1, &
       shape_dims, start_dims, count_dims, &
       adios2_constant_dims, ierr)

  call adios2_open(sstWriter, ioWrite, filename, adios2_mode_write, ierr)

  !Put array contents to bp buffer, based on var1 metadata
  do i = 1, insteps
     call GenerateTestData(i - 1, irank, isize)
     call adios2_begin_step(sstWriter, adios2_step_mode_append, 0.0, &
                            status, ierr)
     call adios2_put(sstWriter, variables(12), data_scalar_r64, ierr)
     call adios2_put(sstWriter, variables(1), data_I8, ierr)
     call adios2_put(sstWriter, variables(2), data_I16, ierr)
     call adios2_put(sstWriter, variables(3), data_I32, ierr)
     call adios2_put(sstWriter, variables(4), data_I64, ierr)
     call adios2_put(sstWriter, variables(5), data_R32, ierr)
     call adios2_put(sstWriter, variables(6), data_R64, ierr)
     call adios2_put(sstWriter, variables(7), data_R64_2d, ierr)
     call adios2_put(sstWriter, variables(8), data_R64_2d_rev, ierr)
     localtime = 0    ! should be time(), but non-portable and value is unused
     call adios2_put(sstWriter, variables(9), loc(localtime), ierr)
     call adios2_put(sstWriter, variables(10), data_C32, ierr)
     call adios2_put(sstWriter, variables(11), data_C64, ierr)
     call adios2_end_step(sstWriter, ierr)
  end do

  !Closes engine1 and deallocates it, becomes unreachable
  call adios2_close(sstWriter, ierr)

   !Deallocates adios and calls its destructor 
   call adios2_finalize(adios, ierr)

#ifdef ADIOS2_HAVE_MPI_F
  call MPI_Finalize(ierr)
#endif

 end program TestSstWrite
