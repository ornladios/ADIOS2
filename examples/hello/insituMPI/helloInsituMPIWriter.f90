program helloInsituMPIWriter
    use helloInsituArgs
    use mpi
    use adios2

    implicit none

    integer(kind=8), dimension(2) :: shape_dims, start_dims, count_dims
    real, dimension(:,:), allocatable :: myArray
    integer :: wrank, wsize, rank, nproc
    integer :: ierr, step = 1
    integer(kind=8) :: i, j
    type(adios2_adios):: adios
    type(adios2_io):: io
    type(adios2_variable):: varArray
    type(adios2_engine):: engine
    integer :: comm, color


    ! Launch MPI
    call MPI_Init(ierr)
    call MPI_Comm_rank(MPI_COMM_WORLD, wrank, ierr)
    call MPI_Comm_size(MPI_COMM_WORLD, wsize, ierr)

    ! Split MPI world into writers and readers. This code is the writers.
    color = 1
    call MPI_Comm_split(MPI_COMM_WORLD, color, wrank, comm, ierr)
    call MPI_Comm_rank(comm, rank, ierr)
    call MPI_Comm_size(comm, nproc, ierr)

    call ProcessArgs(nproc, .true.)

    ! determine offsets
    posx = mod(rank, npx)     ! 1st dim easy: 0, npx, 2npx... are in the same X position
    posy = rank/(npx)         ! 2nd dim: npx consecutive processes belong into one dim
    offx = posx * ndx
    offy = posy * ndy

    ! Application variables
    allocate( myArray(ndx,ndy) )
    myArray = REAL(step*npx*ndx*npy*ndy + rank*ndx*ndy)
    do j=1,ndy
        do i=1,ndx
            myArray(i,j) = myArray(i,j) + REAL((j-1)*ndx + i-1)
        end do
    end do


    ! Variable dimensions
    shape_dims = (/ npx*ndx, npy*ndy /)
    start_dims = (/ offx, offy /)
    count_dims = (/ ndx, ndy /)

    ! Create adios handler passing the communicator, config file, debug mode and error flag
    call adios2_init(adios, xmlfile, comm, adios2_debug_mode_on, ierr)

    ! Declare an IO process configuration inside adios,
    ! Engine choice and parameters for 'writer' come from the config file
    call adios2_declare_io(io, adios, "writer", ierr)

    ! Defines a 2D array variable
    call adios2_define_variable(varArray, io, "myArray", adios2_type_real, &
                                2, shape_dims, start_dims, count_dims, &
                                adios2_constant_dims, ierr)

    ! Open myVector_f.bp in write mode, this launches an engine
    call adios2_open(engine, io, streamname, adios2_mode_write, ierr)

    do step = 1, steps

        do j=1,ndy
            do i=1,ndx
                myArray(i,j) = myArray(i,j) + 0.01
            end do
        end do

        call adios2_begin_step(engine, ierr)
        call adios2_put(engine, varArray, myArray, ierr)
        call adios2_end_step(engine, ierr)

        ! sleep(sleeptime)
    end do

    ! Closes engine and deallocates it, becomes unreachable
    call adios2_close(engine, ierr)

    ! Deallocates adios and calls its destructor
    call adios2_finalize(adios, ierr)

    if( allocated(myArray) ) deallocate(myArray)

    call MPI_Finalize(ierr)

end program helloInsituMPIWriter
