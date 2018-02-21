 program TestBPWriteTypes
     use small_test_data
     use mpi
     use adios2
     implicit none

     integer(kind=8), dimension(1) :: shape_dims, start_dims, count_dims
     integer :: inx, irank, isize, ierr, i

     integer(kind=8) :: adios, ioWrite, bpWriter, ioRead, bpReader
     integer(kind=8), dimension(12) :: variables
     ! read handlers
     character(len=:), allocatable :: variable_name
     integer :: variable_type, ndims
     integer(kind=8), dimension(:), allocatable :: shape_in

     ! Launch MPI
     call MPI_Init(ierr)
     call MPI_Comm_rank(MPI_COMM_WORLD, irank, ierr)
     call MPI_Comm_size(MPI_COMM_WORLD, isize, ierr)

     ! Application variables
     inx = 10

     ! Variable dimensions
     shape_dims(1) = isize*inx
     start_dims(1) = irank*inx
     count_dims(1) = inx

     ! Create adios handler passing the communicator, debug mode and error flag
     call adios2_init(adios, MPI_COMM_WORLD, adios2_debug_mode_on, ierr)

     !!!!!!!!!!!!!!!!!!!!!!!!! WRITER !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
     ! Declare an IO process configuration inside adios
     call adios2_declare_io(ioWrite, adios, "ioWrite", ierr)

     ! Defines a variable to be written in bp format
     call adios2_define_variable(variables(1), ioWrite, "var_I8", 1, &
                                 shape_dims, start_dims, count_dims, &
                                 adios2_constant_dims, data_I8, ierr)

     call adios2_define_variable(variables(2), ioWrite, "var_I16", 1, &
                                 shape_dims, start_dims, count_dims, &
                                 adios2_constant_dims, data_I16, ierr)

     call adios2_define_variable(variables(3), ioWrite, "var_I32", 1, &
                                 shape_dims, start_dims, count_dims, &
                                 adios2_constant_dims, data_I32, ierr)

     call adios2_define_variable(variables(4), ioWrite, "var_I64", 1, &
                                 shape_dims, start_dims, count_dims, &
                                 adios2_constant_dims, data_I64, ierr)

     call adios2_define_variable(variables(5), ioWrite, "var_R32", 1, &
                                 shape_dims, start_dims, count_dims, &
                                 adios2_constant_dims, data_R32, ierr)

     call adios2_define_variable(variables(6), ioWrite, "var_R64", 1, &
                                 shape_dims, start_dims, count_dims, &
                                 adios2_constant_dims, data_R64, ierr)

     ! Global variables
     call adios2_define_variable(variables(7), ioWrite, "gvar_I8", data_I8(1), &
                                 ierr)

     call adios2_define_variable(variables(8), ioWrite, "gvar_I16", &
                                 data_I16(1), ierr)

     call adios2_define_variable(variables(9), ioWrite, "gvar_I32", &
                                 data_I32(1), ierr)

     call adios2_define_variable(variables(10), ioWrite, "gvar_I64", &
                                 data_I64(1), ierr)

     call adios2_define_variable(variables(11), ioWrite, "gvar_R32", &
                                 data_R32(1), ierr)

     call adios2_define_variable(variables(12), ioWrite, "gvar_R64", &
                                 data_R64(1), ierr)

     ! Open myVector_f.bp in write mode, this launches an engine
     call adios2_open(bpWriter, ioWrite, "ftypes.bp", adios2_mode_write, ierr)

     ! Put array contents to bp buffer, based on var1 metadata
     if (irank == 0) then
         call adios2_put_deferred(bpWriter, "gvar_I8", data_I8(1), ierr)
         call adios2_put_deferred(bpWriter, "gvar_I16", data_I16(1), ierr)
         call adios2_put_deferred(bpWriter, "gvar_I32", data_I32(1), ierr)
         call adios2_put_deferred(bpWriter, "gvar_I64", data_I64(1), ierr)
         call adios2_put_deferred(bpWriter, "gvar_R32", data_R32(1), ierr)
         call adios2_put_deferred(bpWriter, "gvar_R64", data_R64(1), ierr)
     end if

     do i = 1, 3
         call adios2_write_step(bpWriter, ierr)
     end do

     ! Closes engine1 and deallocates it, becomes unreachable
     call adios2_close(bpWriter, ierr)

     !!!!!!!!!!!!!!!!!!!!!!!!! READER !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
     ! Declare io reader
     call adios2_declare_io(ioRead, adios, "ioRead", ierr)
     ! Open bpReader engine
     call adios2_open(bpReader, ioRead, "ftypes.bp", adios2_mode_read, ierr)

     call adios2_inquire_variable(variables(1), ioRead, "var_I8", ierr)
     call adios2_variable_name(variables(1), variable_name, ierr)
     if (variable_name /= 'var_I8') stop'var_I8 not recognized'
     call adios2_variable_type(variables(1), variable_type, ierr)
     if (variable_type /= adios2_type_integer1) stop'var_I8 type not recognized'
     call adios2_variable_shape(variables(1), ndims, shape_in, ierr)
     if (ndims /= 1) stop'var_I8 ndims is not 1'
     if (shape_in(1) /= isize*inx) stop'var_I8 shape_in read failed'

     call adios2_inquire_variable(variables(2), ioRead, "var_I16", ierr)
     call adios2_variable_name(variables(2), variable_name, ierr)
     if (variable_name /= 'var_I16') stop'var_I16 not recognized'
     call adios2_variable_type(variables(2), variable_type, ierr)
     if (variable_type /= adios2_type_integer2) stop'var_I16 type not recognized'
     call adios2_variable_shape(variables(2), ndims, shape_in, ierr)
     if (ndims /= 1) stop'var_I16 ndims is not 1'
     if (shape_in(1) /= isize*inx) stop'var_I16 shape_in read failed'

     call adios2_inquire_variable(variables(3), ioRead, "var_I32", ierr)
     call adios2_variable_name(variables(3), variable_name, ierr)
     if (variable_name /= 'var_I32') stop'var_I32 not recognized'
     call adios2_variable_type(variables(3), variable_type, ierr)
     if (variable_type /= adios2_type_integer4) stop'var_I32 type not recognized'
     call adios2_variable_shape(variables(3), ndims, shape_in, ierr)
     if (ndims /= 1) stop'var_I32 ndims is not 1'
     if (shape_in(1) /= isize*inx) stop'var_I32 shape_in read failed'

     call adios2_inquire_variable(variables(4), ioRead, "var_I64", ierr)
     call adios2_variable_name(variables(4), variable_name, ierr)
     if (variable_name /= 'var_I64') stop'var_I64 not recognized'
     call adios2_variable_type(variables(4), variable_type, ierr)
     if (variable_type /= adios2_type_integer8) stop'var_I64 type not recognized'
     call adios2_variable_shape(variables(4), ndims, shape_in, ierr)
     if (ndims /= 1) stop'var_I64 ndims is not 1'
     if (shape_in(1) /= isize*inx) stop'var_I64 shape_in read failed'

     call adios2_inquire_variable(variables(5), ioRead, "var_R32", ierr)
     call adios2_variable_name(variables(5), variable_name, ierr)
     if (variable_name /= 'var_R32') stop'var_R32 not recognized'
     call adios2_variable_type(variables(5), variable_type, ierr)
     if (variable_type /= adios2_type_real) stop'var_R32 type not recognized'
     call adios2_variable_shape(variables(5), ndims, shape_in, ierr)
     if (ndims /= 1) stop'var_R32 ndims is not 1'
     if (shape_in(1) /= isize*inx) stop'var_R32 shape_in read failed'

     call adios2_inquire_variable(variables(6), ioRead, "var_R64", ierr)
     call adios2_variable_name(variables(6), variable_name, ierr)
     if (variable_name /= 'var_R64') stop'var_R64 not recognized'
     call adios2_variable_type(variables(6), variable_type, ierr)
     if (variable_type /= adios2_type_dp) stop'var_R64 type not recognized'
     call adios2_variable_shape(variables(6), ndims, shape_in, ierr)
     if (ndims /= 1) stop'var_R64 ndims is not 1'
     if (shape_in(1) /= isize*inx) stop'var_R64 shape_in read failed'

     call adios2_close(bpReader, ierr)

     ! Deallocates adios and calls its destructor
     call adios2_finalize(adios, ierr)

     call MPI_Finalize(ierr)

 end program TestBPWriteTypes
