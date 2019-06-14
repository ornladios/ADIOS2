!!***************************
subroutine usage()
    print *, "Usage: TestCommonReadF engine filename"
end subroutine usage


program TestSstRead
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
  integer:: irank, isize, ierr, i, insteps, status

  character(len=256) :: filename, engine

  integer :: writerSize, myStart, myLength

  type(adios2_adios)::adios
  type(adios2_io)::ioWrite, ioRead
  type(adios2_variable), dimension(20)::variables
  type(adios2_engine)::sstReader;

  !read handlers
  character(len =:), allocatable::variable_name 
  integer::variable_type, ndims, testComm
  integer(kind = 8), dimension(:), allocatable::shape_in
  integer::key, color
  
  numargs = iargc()

  if ( numargs < 2 ) then
     call usage()
     call exit(1)
  endif


  call getarg(1, engine)
  call getarg(2, filename)

  insteps = 1;

#ifdef ADIOS2_HAVE_MPI_F
  !Launch MPI
  call MPI_Init(ierr) 

  call MPI_Comm_rank(MPI_COMM_WORLD, key, ierr);

  color = 2
  call MPI_Comm_split(MPI_COMM_WORLD, color, key, testComm, ierr);

  call MPI_Comm_rank(testComm, irank, ierr) 
  call MPI_Comm_size(testComm, isize, ierr)

  !Create adios handler passing the communicator, debug mode and error flag
  call adios2_init(adios, testComm, adios2_debug_mode_on, ierr)
#else
  irank = 0;
  isize = 1;

  !Create adios handler passing the communicator, debug mode and error flag
  call adios2_init_nompi(adios, adios2_debug_mode_on, ierr)
#endif
  !!!!!!!!!!!!!!!!!!!!!!!!! READER !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  ! Declare io reader
  call adios2_declare_io(ioRead, adios, "ioRead", ierr)

  call adios2_set_engine(ioRead, engine, ierr)

  ! Open sstReader engine
  call adios2_open(sstReader, ioRead, filename, adios2_mode_read, ierr)

  call adios2_begin_step(sstReader, ierr)

  call adios2_inquire_variable(variables(1), ioRead, "i8", ierr)
  if (variables(1)%name /= 'i8') stop 'i8 not recognized'
  if (variables(1)%type /= adios2_type_integer1) stop 'i8 type not recognized'
  call adios2_variable_shape(shape_in, ndims, variables(1),  ierr)
  if (ndims /= 1) stop 'i8 ndims is not 1'
  if (modulo(shape_in(1), int(nx, 8)) /= 0) stop 'i8 shape_in read failed'
  writerSize = shape_in(1) / nx

  call adios2_inquire_variable(variables(2), ioRead, "i16", ierr)
  if (variables(2)%name /= 'i16') stop 'i16 not recognized'
  if (variables(2)%type /= adios2_type_integer2) stop 'i16 type not recognized'
  call adios2_variable_shape( shape_in, ndims, variables(2), ierr)
  if (ndims /= 1) stop 'i16 ndims is not 1'
  if (shape_in(1) /= nx*writerSize) stop 'i16 shape_in read failed'
  
  call adios2_inquire_variable(variables(3), ioRead, "i32", ierr)
  if (variables(3)%name /= 'i32') stop 'i32 not recognized'
  if (variables(3)%type /= adios2_type_integer4) stop 'i32 type not recognized'
  call adios2_variable_shape(shape_in, ndims, variables(3), ierr)
  if (ndims /= 1) stop 'i32 ndims is not 1'
  if (shape_in(1) /= nx*writerSize) stop 'i32 shape_in read failed'

  call adios2_inquire_variable(variables(4), ioRead, "i64", ierr)
  if (variables(4)%name /= 'i64') stop 'i64 not recognized'
  if (variables(4)%type /= adios2_type_integer8) stop 'i64 type not recognized'
  call adios2_variable_shape(shape_in, ndims, variables(4), ierr)
  if (ndims /= 1) stop 'i64 ndims is not 1'
  if (shape_in(1) /= nx*writerSize) stop 'i64 shape_in read failed'
  
  call adios2_inquire_variable(variables(5), ioRead, "r32", ierr)
  if (variables(5)%name /= 'r32') stop 'r32 not recognized'
  if (variables(5)%type /= adios2_type_real) stop 'r32 type not recognized'
  call adios2_variable_shape(shape_in, ndims, variables(5), ierr)
  if (ndims /= 1) stop 'r32 ndims is not 1'
  if (shape_in(1) /= nx*writerSize) stop 'r32 shape_in read failed'

  call adios2_inquire_variable(variables(6), ioRead, "r64", ierr)
  if (variables(6)%name /= 'r64') stop 'r64 not recognized'
  if (variables(6)%type /= adios2_type_dp) stop 'r64 type not recognized'
  call adios2_variable_shape(shape_in, ndims, variables(6), ierr)
  if (ndims /= 1) stop 'r64 ndims is not 1'
  if (shape_in(1) /= nx*writerSize) stop 'r64 shape_in read failed'

  call adios2_inquire_variable(variables(7), ioRead, "r64_2d", ierr)
  if (variables(7)%name /= 'r64_2d') stop 'r64_2d not recognized'
  if (variables(7)%type /= adios2_type_dp) stop 'r64_2d type not recognized'
  call adios2_variable_shape( shape_in, ndims, variables(7), ierr)
  if (ndims /= 2) stop 'r64_2d ndims is not 2'
  if (shape_in(1) /= 2) stop 'r64_2d shape_in(1) read failed'
  if (shape_in(2) /= nx*writerSize) stop 'r64_2d shape_in(2) read failed'

  call adios2_inquire_variable(variables(8), ioRead, "r64_2d_rev", ierr)
  if (variables(8)%name /= 'r64_2d_rev') stop 'r64_2d_rev not recognized'
  if (variables(8)%type /= adios2_type_dp) stop 'r64_2d_rev type not recognized'
  call adios2_variable_shape(shape_in, ndims, variables(8), ierr)
  if (ndims /= 2) stop 'r64_2d_rev ndims is not 2'
  if (shape_in(1) /= nx*writerSize) stop 'r64_2d_rev shape_in(2) read failed'
  if (shape_in(2) /= 2) stop 'r64_2d_rev shape_in(1) read failed'

  call adios2_inquire_variable(variables(10), ioRead, "c32", ierr)
  if (variables(10)%name /= 'c32') stop 'c32 not recognized'
  if (variables(10)%type /= adios2_type_complex) stop 'c32 type not recognized'
  call adios2_variable_shape(shape_in, ndims, variables(10), ierr)
  if (ndims /= 1) stop 'c32 ndims is not 1'
  if (shape_in(1) /= nx*writerSize) stop 'c32 shape_in read failed'

  call adios2_inquire_variable(variables(11), ioRead, "c64", ierr)
  if (variables(11)%name /= 'c64') stop 'c64 not recognized'
  if (variables(11)%type /= adios2_type_complex_dp) stop 'c64 type not recognized'
  call adios2_variable_shape(shape_in, ndims, variables(11), ierr)
  if (ndims /= 1) stop 'c64 ndims is not 1'
  if (shape_in(1) /= nx*writerSize) stop 'c64 shape_in read failed'

  call adios2_inquire_variable(variables(12), ioRead, "scalar_r64", ierr)
  if (variables(12)%name /= 'scalar_r64') stop 'scalar_r64 not recognized'
  if (variables(12)%type /= adios2_type_dp) stop 'scalar_r64 type not recognized'
  
  myStart = (writerSize * Nx / isize) * irank
  myLength = ((writerSize * Nx + isize - 1) / isize)

  if (myStart + myLength > writerSize * Nx) then
      myLength = writerSize * Nx - myStart
  end if
  allocate (in_I8(myLength));
  allocate (in_I16(myLength));
  allocate (in_I32(myLength));
  allocate (in_I64(myLength));
  allocate (in_R32(myLength));
  allocate (in_R64(myLength));
  allocate (in_C32(myLength));
  allocate (in_C64(myLength));
  allocate (in_R64_2d(2, myLength));
  allocate (in_R64_2d_rev(myLength, 2));

  start_dims(1) = myStart
  count_dims(1) = myLength

  start_dims2(1) = 0
  count_dims2(1) = 2
  start_dims2(2) = myStart
  count_dims2(2) = myLength

  start_dims3(1) = myStart
  count_dims3(1) = myLength
  start_dims3(2) = 0
  count_dims3(2) = 2

  call adios2_set_selection(variables(1), 1, start_dims, count_dims, ierr)
  call adios2_set_selection(variables(2), 1, start_dims, count_dims, ierr)
  call adios2_set_selection(variables(3), 1, start_dims, count_dims, ierr)
  call adios2_set_selection(variables(4), 1, start_dims, count_dims, ierr)
  call adios2_set_selection(variables(5), 1, start_dims, count_dims, ierr)
  call adios2_set_selection(variables(6), 1, start_dims, count_dims, ierr)
  call adios2_set_selection(variables(7), 2, start_dims2, count_dims2, ierr)
  call adios2_set_selection(variables(8), 2, start_dims3, count_dims3, ierr)
  call adios2_set_selection(variables(10), 1, start_dims, count_dims, ierr)
  call adios2_set_selection(variables(11), 1, start_dims, count_dims, ierr)

  call adios2_get(sstReader, variables(1), in_I8, ierr)
  call adios2_get(sstReader, variables(2), in_I16, ierr)
  call adios2_get(sstReader, variables(3), in_I32, ierr)
  call adios2_get(sstReader, variables(4), in_I64, ierr)
  call adios2_get(sstReader, variables(5), in_R32, ierr)
  call adios2_get(sstReader, variables(6), in_R64, ierr)
  call adios2_get(sstReader, variables(7), in_R64_2d, ierr)
  call adios2_get(sstReader, variables(8), in_R64_2d_rev, ierr)
  call adios2_get(sstReader, variables(10), in_C32, ierr)
  call adios2_get(sstReader, variables(11), in_C64, ierr)
  call adios2_get(sstReader, variables(12), in_scalar_R64, ierr)

  call adios2_end_step(sstReader, ierr)
  
  call validateTestData(myStart, myLength, 0)

  call adios2_close(sstReader, ierr)

  ! Deallocates adios and calls its destructor
  call adios2_finalize(adios, ierr)
  if( adios%valid .eqv. .true. ) stop 'Invalid adios2_finalize'


#ifdef ADIOS2_HAVE_MPI_F
  call MPI_Finalize(ierr)
#endif


 end program TestSstRead
