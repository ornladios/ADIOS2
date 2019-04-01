 program TestBPWriteTypes
     use small_test_data
     use mpi
     use adios2
     implicit none

     integer(kind=8), dimension(1) :: shape_dims, start_dims, count_dims
     integer :: inx, irank, isize, ierr, i, step_status

     type(adios2_adios) :: adios
     type(adios2_io) :: ioWrite, ioRead
     type(adios2_variable), dimension(13) :: variables
     type(adios2_engine) :: bpWriter, bpReader
     character(len=15) :: inString
     character(len=:), allocatable :: varName
     character(len=:), allocatable :: engineType

     ! read handlers
     integer :: ndims
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

     if( adios%valid .eqv. .true. ) stop 'Invalid adios default'
     if( ioWrite%valid .eqv. .true. ) stop 'Invalid io default'

     do i=1,12
        if( variables(i)%valid .eqv. .true. ) stop 'Invalid variables default'
     end do

     if( bpWriter%valid .eqv. .true. ) stop 'Invalid engine default'


     ! Create adios handler passing the communicator, debug mode and error flag
     call adios2_init(adios, MPI_COMM_WORLD, adios2_debug_mode_on, ierr)
     if( adios%valid .eqv. .false. ) stop 'Invalid adios2_init'

     !!!!!!!!!!!!!!!!!!!!!!!!! WRITER !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
     ! Declare an IO process configuration inside adios
     call adios2_declare_io(ioWrite, adios, "ioWrite", ierr)
     if( ioWrite%valid .eqv. .false. ) stop 'Invalid adios2_declare_io'

     call adios2_at_io(ioWrite, adios, "ioWrite", ierr)
     if( ioWrite%valid .eqv. .false. ) stop 'Invalid adios2_at_io'

     call adios2_set_engine(ioWrite, 'bpfile', ierr)

     ! Defines a variable to be written in bp format
     call adios2_define_variable(variables(1), ioWrite, "var_I8", &
                                 adios2_type_integer1, 1, &
                                 shape_dims, start_dims, count_dims, &
                                 adios2_constant_dims, ierr)

     call adios2_define_variable(variables(2), ioWrite, "var_I16", &
                                 adios2_type_integer2, 1, &
                                 shape_dims, start_dims, count_dims, &
                                 adios2_constant_dims, ierr)

     call adios2_define_variable(variables(3), ioWrite, "var_I32", &
                                 adios2_type_integer4, 1, &
                                 shape_dims, start_dims, count_dims, &
                                 adios2_constant_dims, ierr)

     call adios2_define_variable(variables(4), ioWrite, "var_I64", &
                                 adios2_type_integer8, 1, &
                                 shape_dims, start_dims, count_dims, &
                                 adios2_constant_dims, ierr)

     call adios2_define_variable(variables(5), ioWrite, "var_R32", &
                                 adios2_type_real, 1, &
                                 shape_dims, start_dims, count_dims, &
                                 adios2_constant_dims, ierr)

     call adios2_define_variable(variables(6), ioWrite, "var_R64", &
                                 adios2_type_dp, 1, &
                                 shape_dims, start_dims, count_dims, &
                                 adios2_constant_dims, ierr)

     ! Global variables
     call adios2_define_variable(variables(7), ioWrite, "gvar_I8", &
                                 adios2_type_integer1,  ierr)

     call adios2_define_variable(variables(8), ioWrite, "gvar_I16", &
                                 adios2_type_integer2,  ierr)

     call adios2_define_variable(variables(9), ioWrite, "gvar_I32", &
                                 adios2_type_integer4,  ierr)

     call adios2_define_variable(variables(10), ioWrite, "gvar_I64", &
                                 adios2_type_integer8,  ierr)

     call adios2_define_variable(variables(11), ioWrite, "gvar_R32", &
                                 adios2_type_real,  ierr)

     call adios2_define_variable(variables(12), ioWrite, "gvar_R64", &
                                 adios2_type_dp,  ierr)

     call adios2_define_variable(variables(13), ioWrite, "gvar_Str", &
                                 adios2_type_string, ierr)

     do i=1,13
        if( variables(i)%valid .eqv. .false. ) stop 'Invalid adios2_define_variable'
     end do

     ! Testing adios2_variable_name for just two cases
     call adios2_variable_name(varName, variables(1), ierr)
     if (varName /= 'var_I8') stop 'Invalid adios2_variable_name'

     call adios2_variable_name(varName, variables(2), ierr)
     if (varName /= 'var_I16') stop 'Invalid adios2_variable_name'

     deallocate(varName)

     ! Open myVector_f.bp in write mode, this launches an engine
     if( ioWrite%valid .eqv. .false. ) stop 'Invalid adios2_io'
     if( bpWriter%valid .eqv. .true. ) stop 'Invalid adios2_engine pre-open'

     call adios2_open(bpWriter, ioWrite, "ftypes.bp", adios2_mode_write, ierr)

     if( bpWriter%valid .eqv. .false. ) stop 'Invalid adios2_engine post-open'
     if( TRIM(bpWriter%name) /= 'ftypes.bp') stop 'Invalid adios2_engine name'

     if( TRIM(bpWriter%type) /= 'bpfile') then
        write(*,*) 'Engine Type ', TRIM(bpWriter%type)
        stop 'Invalid adios2_engine name'
     end if
     call adios2_io_engine_type(engineType, ioWrite, ierr)
     if( engineType /= 'bp') then ! FIXME, different from the above!
        write(*,*) 'Engine Type ', engineType
        stop 'Invalid type from adios2_engine_type'
     end if

     if( bpWriter%mode /= adios2_mode_write) stop 'Invalid adios2_engine mode'

     ! Put array contents to bp buffer, based on var1 metadata
     do i = 1, 3
         call adios2_begin_step(bpWriter, adios2_step_mode_append, 0.0, &
                                step_status, ierr)

         if (irank == 0 .and. i == 1) then
           call adios2_put(bpWriter, variables(7), data_I8(1), ierr)
           call adios2_put(bpWriter, variables(8), data_I16(1), ierr)
           call adios2_put(bpWriter, variables(9), data_I32(1), ierr)
           call adios2_put(bpWriter, variables(10), data_I64(1), ierr)
           call adios2_put(bpWriter, variables(11), data_R32(1), ierr)
           call adios2_put(bpWriter, variables(12), data_R64(1), ierr)
           call adios2_put(bpWriter, variables(13), data_Strings(1), ierr)
         end if

         call adios2_put(bpWriter, variables(1), data_I8, ierr)
         call adios2_put(bpWriter, variables(2), data_I16, ierr)
         call adios2_put(bpWriter, variables(3), data_I32, ierr)
         call adios2_put(bpWriter, variables(4), data_I64, ierr)
         call adios2_put(bpWriter, variables(5), data_R32, ierr)
         call adios2_put(bpWriter, variables(6), data_R64, ierr)
         call adios2_end_step(bpWriter, ierr)
     end do

     ! Closes engine1 and deallocates it, becomes unreachable
     call adios2_close(bpWriter, ierr)

     if( bpWriter%valid .eqv. .true. ) stop 'Invalid adios2_close'

     !!!!!!!!!!!!!!!!!!!!!!!!! READER !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
     ! Declare io reader
     call adios2_declare_io(ioRead, adios, "ioRead", ierr)
     ! Open bpReader engine
     call adios2_open(bpReader, ioRead, "ftypes.bp", adios2_mode_read, ierr)

     call adios2_inquire_variable(variables(1), ioRead, "var_I8", ierr)
     if (variables(1)%name /= 'var_I8') stop 'var_I8 not recognized'
     if (variables(1)%type /= adios2_type_integer1) stop 'var_I8 type not recognized'
     call adios2_variable_shape(shape_in, ndims, variables(1), ierr)
     if (ndims /= 1) stop 'var_I8 ndims is not 1'
     if (shape_in(1) /= isize*inx) stop 'var_I8 shape_in read failed'

     call adios2_inquire_variable(variables(2), ioRead, "var_I16", ierr)
     if (variables(2)%name /= 'var_I16') stop 'var_I16 not recognized'
     if (variables(2)%type /= adios2_type_integer2) stop 'var_I16 type not recognized'
     call adios2_variable_shape(shape_in, ndims, variables(2), ierr)
     if (ndims /= 1) stop 'var_I16 ndims is not 1'
     if (shape_in(1) /= isize*inx) stop 'var_I16 shape_in read failed'

     call adios2_inquire_variable(variables(3), ioRead, "var_I32", ierr)
     if (variables(3)%name /= 'var_I32') stop 'var_I32 not recognized'
     if (variables(3)%type /= adios2_type_integer4) stop 'var_I32 type not recognized'
     call adios2_variable_shape(shape_in, ndims, variables(3), ierr)
     if (ndims /= 1) stop 'var_I32 ndims is not 1'
     if (shape_in(1) /= isize*inx) stop 'var_I32 shape_in read failed'

     call adios2_inquire_variable(variables(4), ioRead, "var_I64", ierr)
     if (variables(4)%name /= 'var_I64') stop 'var_I64 not recognized'
     if (variables(4)%type /= adios2_type_integer8) stop 'var_I64 type not recognized'
     call adios2_variable_shape(shape_in, ndims, variables(4), ierr)
     if (ndims /= 1) stop 'var_I64 ndims is not 1'
     if (shape_in(1) /= isize*inx) stop 'var_I64 shape_in read failed'

     call adios2_inquire_variable(variables(5), ioRead, "var_R32", ierr)
     if (variables(5)%name /= 'var_R32') stop 'var_R32 not recognized'
     if (variables(5)%type /= adios2_type_real) stop 'var_R32 type not recognized'
     call adios2_variable_shape(shape_in, ndims, variables(5), ierr)
     if (ndims /= 1) stop 'var_R32 ndims is not 1'
     if (shape_in(1) /= isize*inx) stop 'var_R32 shape_in read failed'

     call adios2_inquire_variable(variables(6), ioRead, "var_R64", ierr)
     if (variables(6)%name /= 'var_R64') stop 'var_R64 not recognized'
     if (variables(6)%type /= adios2_type_dp) stop 'var_R64 type not recognized'
     call adios2_variable_shape(shape_in, ndims, variables(6), ierr)
     if (ndims /= 1) stop 'var_R64 ndims is not 1'
     if (shape_in(1) /= isize*inx) stop 'var_R64 shape_in read failed'

     call adios2_inquire_variable(variables(13), ioRead, "gvar_Str", ierr)
     call adios2_get(bpReader, variables(13), inString, ierr)
     call adios2_perform_gets(bpReader, ierr)
     if( inString /= data_Strings(1) ) stop 'gvar_Str read failed'

     call adios2_close(bpReader, ierr)

     ! Deallocates adios and calls its destructor
     call adios2_finalize(adios, ierr)
     if( adios%valid .eqv. .true. ) stop 'Invalid adios2_finalize'

     call MPI_Finalize(ierr)

 end program TestBPWriteTypes
