program TestBPWriteTypes
    use small_test_data
    use mpi
    use adios2
    implicit none

    integer, dimension(1) :: shape_dims, start_dims, count_dims
    integer :: inx, irank, isize, ierr, i

    integer(kind=8) :: adios, ioWrite, bpWriter, ioRead, bpReader
    integer(kind=8), dimension(6) :: variables
    character(len=:), allocatable :: variable_name, variable_type

    ! Launch MPI
    call MPI_Init(ierr)
    call MPI_Comm_rank(MPI_COMM_WORLD, irank, ierr)
    call MPI_Comm_size(MPI_COMM_WORLD, isize, ierr)

    ! Application variables
    inx = 10

    ! Variable dimensions
    shape_dims(1) = isize * inx
    start_dims(1) = irank * inx
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

    ! Open myVector_f.bp in write mode, this launches an engine
    call adios2_open(bpWriter, ioWrite, "ftypes.bp", adios2_mode_write, ierr)

    ! Put array contents to bp buffer, based on var1 metadata
    call adios2_write_step(bpWriter, ierr)

    ! Closes engine1 and deallocates it, becomes unreachable
    call adios2_close(bpWriter, ierr)


    !!!!!!!!!!!!!!!!!!!!!!!!! READER !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    ! Declare io reader
    call adios2_declare_io(ioRead, adios, "ioRead", ierr)
    ! Open bpReader engine
    call adios2_open(bpReader, ioRead, "ftypes.bp", adios2_mode_read, ierr)

    call adios2_inquire_variable( variables(1), ioRead, "var_I8", ierr)

    if( ierr == adios2_found ) then

        call adios2_variable_name( variables(1), variable_name, ierr )
        if( variable_name /= 'var_I8') then
            stop 'error'
        end if
    end if

    ! Deallocates adios and calls its destructor
    call adios2_finalize(adios, ierr)

    call MPI_Finalize(ierr)

end program TestBPWriteTypes
