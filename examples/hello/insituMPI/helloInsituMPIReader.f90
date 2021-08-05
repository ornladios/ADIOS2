program helloInsituMPIReader
    use helloInsituArgs
    use mpi
    use adios2

    implicit none

    type(adios2_adios):: adios
    type(adios2_io):: io
    type(adios2_variable):: varArray
    type(adios2_engine):: engine

    integer :: wrank, wsize, rank, nproc
    real, dimension(:,:), allocatable :: myArray
    integer :: ndims
    integer(kind=8), dimension(:), allocatable :: shape_dims
    integer(kind=8), dimension(:), allocatable :: sel_start, sel_count
    integer :: ierr
    integer :: step
    integer :: comm, color


    ! Launch MPI
    call MPI_Init(ierr)
    call MPI_Comm_rank(MPI_COMM_WORLD, wrank, ierr)
    call MPI_Comm_size(MPI_COMM_WORLD, wsize, ierr)

    ! Split MPI world into writers and readers. This code is the writers.
    color = 2
    call MPI_Comm_split(MPI_COMM_WORLD, color, wrank, comm, ierr)
    call MPI_Comm_rank(comm, rank, ierr)
    call MPI_Comm_size(comm, nproc, ierr)

    call ProcessArgs(nproc, .false.)

    ! Start adios2
    call adios2_init( adios, xmlfile, comm, adios2_debug_mode_on, ierr )

    ! Declare an IO process configuration inside adios,
    ! Engine choice and parameters for 'writer' come from the config file
    call adios2_declare_io( io, adios, 'reader', ierr )

    call adios2_open( engine, io, streamname, adios2_mode_read, ierr)

    if( ierr == adios2_found ) then
        step = 0
        do
            call adios2_begin_step(engine, ierr)
            if (ierr /= adios2_step_status_ok) then
                exit
            endif

            call adios2_inquire_variable( varArray, io, 'myArray', ierr )

            if (step == 0) then
                call adios2_variable_shape( shape_dims, ndims, varArray, ierr)
                ! ndims is assumed to be 2 here
                call DecomposeArray( shape_dims(1), shape_dims(2), rank)
                allocate (sel_start(2), sel_count(2))
                sel_start = (/ offx, offy /)
                sel_count = (/ ndx, ndy /)
                allocate( myArray( sel_count(1), sel_count(2)) )
            endif

            call adios2_set_selection( varArray, 2, sel_start, sel_count, ierr )
            call adios2_get( engine, varArray, myArray, ierr )
            call adios2_end_step(engine, ierr)

            call print_array(myArray, sel_start, rank, step)

            step = step + 1
        end do


        if( allocated(myArray) ) deallocate(myArray)

    else
        write(*, '("Variable myArray not found in stream! ierr=",i0)') ierr
    end if

    call adios2_close( engine, ierr )
    call adios2_finalize(adios, ierr)
    call MPI_Finalize(ierr)

contains

subroutine print_array(xy,offset,rank, step)
    implicit none
    include 'mpif.h'
    real,    dimension(:,:), intent(in) :: xy
    integer*8, dimension(2),   intent(in) :: offset
    integer,   intent(in)                 :: rank, step

    integer :: size1,size2
    integer :: i,j

    size1 = size(xy,1)
    size2 = size(xy,2)

    write (100+rank, '("rank=",i0," size=",i0,"x",i0," offsets=",i0,":",i0," step=",i0)') &
        rank, size1, size2, offset(1), offset(2), step
    write (100+rank, '(" time   row   columns ",i0,"...",i0)') offset(2), offset(2)+size2-1
    write (100+rank, '("        ",$)')
    do j=1,size2
        write (100+rank, '(i9,$)') offset(2)+j-1
    enddo
    write (100+rank, '(" ")')
    write (100+rank, '("--------------------------------------------------------------")')
    do i=1,size1
        write (100+rank, '(2i5,$)') step,offset(1)+i-1
        do j=1,size2
            write (100+rank, '(f9.2,$)') xy(i,j)
        enddo
        write (100+rank, '(" ")')
    enddo

end subroutine print_array

end program helloInsituMPIReader

