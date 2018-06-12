 program TestBPWriteTypes
     use small_test_data
     use mpi
     use adios2
     implicit none

     integer(kind=8), dimension(1) :: count_dims
     integer :: inx, irank, isize, ierr, i

     type(adios2_adios) :: adios
     type(adios2_io) :: ioWrite, ioRead
     type(adios2_variable), dimension(12) :: variables
     type(adios2_engine) :: bpWriter, bpReader


     ! read handlers
     character(len=:), allocatable :: variable_name, engine_type
     integer :: variable_type, ndims
     integer(kind=8), dimension(:), allocatable :: count_in
     integer(kind=8) steps_start, steps_count, current_step

     ! Launch MPI
     call MPI_Init(ierr)
     call MPI_Comm_rank(MPI_COMM_WORLD, irank, ierr)
     call MPI_Comm_size(MPI_COMM_WORLD, isize, ierr)

     ! Application variables
     inx = 10

     ! Variable dimensions
     count_dims(1) = inx

     ! Create adios handler passing the communicator, debug mode and error flag
     call adios2_init(adios, MPI_COMM_WORLD, adios2_debug_mode_on, ierr)

     !!!!!!!!!!!!!!!!!!!!!!!!! WRITER !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
     ! Declare an IO process configuration inside adios
     call adios2_declare_io(ioWrite, adios, "ioWrite", ierr)

     ! Defines a variable to be written in bp format
     call adios2_define_variable(variables(1), ioWrite, "var_I8", &
                                 adios2_type_integer1, 1, &
                                 adios2_null_dims, adios2_null_dims, count_dims, &
                                 adios2_constant_dims, ierr)

     call adios2_define_variable(variables(2), ioWrite, "var_I16", &
                                 adios2_type_integer2, 1, &
                                 adios2_null_dims, adios2_null_dims, count_dims, &
                                 adios2_constant_dims, ierr)

     call adios2_define_variable(variables(3), ioWrite, "var_I32", &
                                 adios2_type_integer4, 1, &
                                 adios2_null_dims, adios2_null_dims, count_dims, &
                                 adios2_constant_dims, ierr)

     call adios2_define_variable(variables(4), ioWrite, "var_I64", &
                                 adios2_type_integer8, 1, &
                                 adios2_null_dims, adios2_null_dims, count_dims, &
                                 adios2_constant_dims, ierr)

     call adios2_define_variable(variables(5), ioWrite, "var_R32", &
                                 adios2_type_real, 1, &
                                 adios2_null_dims, adios2_null_dims, count_dims, &
                                 adios2_constant_dims,  ierr)

     call adios2_define_variable(variables(6), ioWrite, "var_R64", &
                                 adios2_type_dp, 1, &
                                 adios2_null_dims, adios2_null_dims, count_dims, &
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

     call adios2_io_engine_type( ioWrite, engine_type, ierr)
     write(*,*) "Engine type: ", engine_type

     ! Open myVector_f.bp in write mode, this launches an engine
     call adios2_open(bpWriter, ioWrite, "ftypes_local.bp", adios2_mode_write, ierr)

     do i = 1, 3
         call adios2_begin_step(bpWriter, ierr)

         if (irank == 0 .and. i == 1) then
            call adios2_put(bpWriter, variables(7), data_I8(1), ierr)
            call adios2_put(bpWriter, variables(8), data_I16(1), ierr)
            call adios2_put(bpWriter, variables(9), data_I32(1), ierr)
            call adios2_put(bpWriter, variables(10), data_I64(1), ierr)
            call adios2_put(bpWriter, variables(11), data_R32(1), ierr)
            call adios2_put(bpWriter, variables(12), data_R64(1), ierr)
         end if

         call adios2_put(bpWriter, variables(1), data_I8, ierr)
         call adios2_put(bpWriter, variables(2), data_I16, ierr)
         call adios2_put(bpWriter, variables(3), data_I32, ierr)
         call adios2_put(bpWriter, variables(4), data_I64, ierr)
         call adios2_put(bpWriter, variables(5), data_R32, ierr)
         call adios2_put(bpWriter, variables(6), data_R64, ierr)

         call adios2_current_step(bpWriter, current_step, ierr)
         if (current_step /= i-1) stop 'wrong current step'

         call adios2_end_step(bpWriter, ierr)
     end do

     ! Closes engine1 and deallocates it, becomes unreachable
     call adios2_close(bpWriter, ierr)

     !!!!!!!!!!!!!!!!!!!!!!!!! READER !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
     ! Declare io reader
     call adios2_declare_io(ioRead, adios, "ioRead", ierr)
     ! Open bpReader engine
     call adios2_open(bpReader, ioRead, "ftypes_local.bp", adios2_mode_read, ierr)

     call adios2_inquire_variable(variables(1), ioRead, "var_I8", ierr)
     call adios2_variable_name(variables(1), variable_name, ierr)
     if (variable_name /= 'var_I8') stop 'var_I8 not recognized'
     call adios2_variable_type(variables(1), variable_type, ierr)
     if (variable_type /= adios2_type_integer1) stop 'var_I8 type not recognized'
     call adios2_variable_steps_start(variables(1), steps_start, ierr)
     if (steps_start /= 0) stop 'var_I8 steps_start is not 0'
     call adios2_variable_steps(variables(1), steps_count, ierr)
     if (steps_count /= 3) stop 'var_I8 steps_count is not 3'


     call adios2_inquire_variable(variables(2), ioRead, "var_I16", ierr)
     call adios2_variable_name(variables(2), variable_name, ierr)
     if (variable_name /= 'var_I16') stop 'var_I16 not recognized'
     call adios2_variable_type(variables(2), variable_type, ierr)
     if (variable_type /= adios2_type_integer2) stop 'var_I16 type not recognized'
     call adios2_variable_steps_start(variables(2), steps_start, ierr)
     if (steps_start /= 0) stop 'var_I16 steps_start is not 0'
     call adios2_variable_steps(variables(2), steps_count, ierr)
     if (steps_count /= 3) stop 'var_I16 steps_count is not 3'


     call adios2_inquire_variable(variables(3), ioRead, "var_I32", ierr)
     call adios2_variable_name(variables(3), variable_name, ierr)
     if (variable_name /= 'var_I32') stop 'var_I32 not recognized'
     call adios2_variable_type(variables(3), variable_type, ierr)
     if (variable_type /= adios2_type_integer4) stop 'var_I32 type not recognized'
     call adios2_variable_steps_start(variables(3), steps_start, ierr)
     if (steps_start /= 0) stop 'var_I32 steps_start is not 0'
     call adios2_variable_steps(variables(3), steps_count, ierr)
     if (steps_count /= 3) stop 'var_I32 steps_count is not 3'

     call adios2_inquire_variable(variables(4), ioRead, "var_I64", ierr)
     call adios2_variable_name(variables(4), variable_name, ierr)
     if (variable_name /= 'var_I64') stop 'var_I64 not recognized'
     call adios2_variable_type(variables(4), variable_type, ierr)
     if (variable_type /= adios2_type_integer8) stop 'var_I64 type not recognized'
     call adios2_variable_steps_start(variables(4), steps_start, ierr)
     if (steps_start /= 0) stop 'var_I64 steps_start is not 0'
     call adios2_variable_steps(variables(4), steps_count, ierr)
     if (steps_count /= 3) stop 'var_I64 steps_count is not 3'

     call adios2_inquire_variable(variables(5), ioRead, "var_R32", ierr)
     call adios2_variable_name(variables(5), variable_name, ierr)
     if (variable_name /= 'var_R32') stop 'var_R32 not recognized'
     call adios2_variable_type(variables(5), variable_type, ierr)
     if (variable_type /= adios2_type_real) stop 'var_R32 type not recognized'
     call adios2_variable_steps_start(variables(5), steps_start, ierr)
     if (steps_start /= 0) stop 'var_R32 steps_start is not 0'
     call adios2_variable_steps(variables(5), steps_count, ierr)
     if (steps_count /= 3) stop 'var_R32 steps_count is not 3'

     call adios2_inquire_variable(variables(6), ioRead, "var_R64", ierr)
     call adios2_variable_name(variables(6), variable_name, ierr)
     if (variable_name /= 'var_R64') stop 'var_R64 not recognized'
     call adios2_variable_type(variables(6), variable_type, ierr)
     if (variable_type /= adios2_type_dp) stop 'var_R64 type not recognized'
     call adios2_variable_steps_start(variables(6), steps_start, ierr)
     if (steps_start /= 0) stop 'var_R64 steps_start is not 0'
     call adios2_variable_steps(variables(6), steps_count, ierr)
     if (steps_count /= 3) stop 'var_R64 steps_count is not 3'

     call adios2_inquire_variable(variables(7), ioRead, "gvar_I8", ierr)
     call adios2_variable_name(variables(7), variable_name, ierr)
     if (variable_name /= 'gvar_I8') stop 'gvar_I8 not recognized'
     call adios2_variable_steps_start(variables(7), steps_start, ierr)
     if (steps_start /= 0) stop 'gvar_I8 steps_start is not 0'
     call adios2_variable_steps(variables(7), steps_count, ierr)
     if (steps_count /= 1) stop 'gvar_I8 steps_count is not 1'

     call adios2_inquire_variable(variables(8), ioRead, "gvar_I16", ierr)
     call adios2_variable_name(variables(8), variable_name, ierr)
     if (variable_name /= 'gvar_I16') stop 'gvar_I16 not recognized'
     call adios2_variable_steps_start(variables(8), steps_start, ierr)
     if (steps_start /= 0) stop 'gvar_I16 steps_start is not 0'
     call adios2_variable_steps(variables(8), steps_count, ierr)
     if (steps_count /= 1) stop 'gvar_I16 steps_count is not 1'

     call adios2_inquire_variable(variables(9), ioRead, "gvar_I32", ierr)
     call adios2_variable_name(variables(9), variable_name, ierr)
     if (variable_name /= 'gvar_I32') stop 'gvar_I32 not recognized'
     call adios2_variable_steps_start(variables(9), steps_start, ierr)
     if (steps_start /= 0) stop 'gvar_I32 steps_start is not 0'
     call adios2_variable_steps(variables(9), steps_count, ierr)
     if (steps_count /= 1) stop 'gvar_I32 steps_count is not 1'

     call adios2_inquire_variable(variables(10), ioRead, "gvar_I64", ierr)
     call adios2_variable_name(variables(10), variable_name, ierr)
     if (variable_name /= 'gvar_I64') stop 'gvar_I32 not recognized'
     call adios2_variable_steps_start(variables(10), steps_start, ierr)
     if (steps_start /= 0) stop 'gvar_I64 steps_start is not 0'
     call adios2_variable_steps(variables(10), steps_count, ierr)
     if (steps_count /= 1) stop 'gvar_I64 steps_count is not 1'

     call adios2_inquire_variable(variables(11), ioRead, "gvar_R32", ierr)
     call adios2_variable_name(variables(11), variable_name, ierr)
     if (variable_name /= 'gvar_R32') stop 'gvar_R32 not recognized'
     call adios2_variable_steps_start(variables(11), steps_start, ierr)
     if (steps_start /= 0) stop 'gvar_R32 steps_start is not 0'
     call adios2_variable_steps(variables(11), steps_count, ierr)
     if (steps_count /= 1) stop 'gvar_R32 steps_count is not 1'

     call adios2_inquire_variable(variables(12), ioRead, "gvar_R64", ierr)
     call adios2_variable_name(variables(12), variable_name, ierr)
     if (variable_name /= 'gvar_R64') stop 'gvar_R64 not recognized'
     call adios2_variable_steps_start(variables(12), steps_start, ierr)
     if (steps_start /= 0) stop 'gvar_R64 steps_start is not 0'
     call adios2_variable_steps(variables(12), steps_count, ierr)
     if (steps_count /= 1) stop 'gvar_R64 steps_count is not 1'

     call adios2_close(bpReader, ierr)

     ! Deallocates adios and calls its destructor
     call adios2_finalize(adios, ierr)

     call MPI_Finalize(ierr)

 end program TestBPWriteTypes
