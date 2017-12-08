program helloBPWriter
    use mpi
    use adios2

    implicit none

    integer, dimension(1) :: shape_dims, start_dims, count_dims
    real, dimension(:), allocatable :: myArray
    integer :: inx, irank, isize, ierr, i, var1_type
    integer(kind=8) :: adios, io, var1, engine1
    character(len=:), allocatable :: var1_name


    ! Launch MPI
    call MPI_Init(ierr)
    call MPI_Comm_rank(MPI_COMM_WORLD, irank, ierr)
    call MPI_Comm_size(MPI_COMM_WORLD, isize, ierr)

    ! Application variables
    inx = 10
    allocate( myArray(inx) )

    do i=1,inx
        myArray(i) = i-1
    end do

    ! Variable dimensions
    shape_dims(1) = isize * inx
    start_dims(1) = irank * inx
    count_dims(1) = inx

    ! Create adios handler passing the communicator, debug mode and error flag
    call adios2_init(adios, MPI_COMM_WORLD, adios2_debug_mode_on, ierr)

    ! Declare an IO process configuration inside adios
    call adios2_declare_io(io, adios, "bpIO", ierr)

    ! Defines a variable to be written in bp format
    call adios2_define_variable(var1, io, "myArray", adios2_type_real, 1, &
        & shape_dims, start_dims, count_dims, adios2_constant_dims_true, ierr)


    call adios2_variable_name( var1, var1_name, ierr )
    call adios2_variable_type( var1, var1_type, ierr )
    write(*,*) 'Variable name: ', var1_name, '  type: ', var1_type

    ! Open myVector_f.bp in write mode, this launches an engine
    call adios2_open(engine1, io, "myVector_f.bp", adios2_mode_write, ierr)

    ! Put myArray contents to bp buffer, based on var1 metadata
    call adios2_put_sync(engine1, var1, myArray, ierr)

    ! Closes engine1 and deallocates it, becomes unreachable
    call adios2_close(engine1, ierr)

    ! Deallocates adios and calls its destructor
    call adios2_finalize(adios, ierr)

    if( allocated(myArray) ) deallocate(myArray)
    if( allocated(var1_name) ) deallocate(var1_name)

    call MPI_Finalize(ierr)

end program helloBPWriter
