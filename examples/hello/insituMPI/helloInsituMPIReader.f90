program helloInsituMPIReader
    use helloInsituArgs
    use mpi
    use adios2

    implicit none

    integer(kind=8) :: adios
    integer(kind=8) :: io, varArray, engine
    integer :: wrank, wsize, rank, nproc
    integer, dimension(:,:), allocatable :: myArray
    integer(kind=8), dimension(2) :: ishape, istart, icount
    integer(kind=8), dimension(2) :: sel_start, sel_count
    integer :: ierr
    integer :: i, j
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

    call ProcessArgs(rank, nproc, .false.)

    ! Start adios2
    call adios2_init_config( adios, xmlfile, MPI_COMM_WORLD, adios2_debug_mode_on, ierr )

    ! Declare an IO process configuration inside adios,
    ! Engine choice and parameters for 'writer' come from the config file
    call adios2_declare_io( io, adios, 'reader', ierr )

    call adios2_open( engine, io, streamname, adios2_mode_read, ierr)

    call adios2_inquire_variable( varArray, io, 'myArray', ierr )

    if( ierr == adios2_found ) then

        ! FIXME: No way to get the shape of varArray at the moment
        sel_start = (/ rank, 0 /)
        sel_count = (/ 1, 1 /)
        allocate( myArray( sel_count(1), sel_count(2)) )

        call adios2_set_selection( varArray, 2, sel_start, sel_count, ierr )

        do
            call adios2_begin_step(engine, adios2_step_mode_next_available, 0.0, ierr)
            if (ierr /= adios2_step_status_ok) then
                exit
            endif

            call adios2_get_deferred( engine, varArray, myArray, ierr )
            call adios2_end_step(engine, ierr)

            write(*,'(A,2(I1,A),A,2(I1,A),A)') 'Data selection  &
                      & [ start = ('        , (sel_start(i),',',i=1,2) , ') &
                      &  count =  ('        , (sel_count(i),',',i=1,2) , ') ]'

            do j=1,sel_count(2)
                do i=1,sel_count(1)
                    write(6,'(I5) ', advance="no") myArray(i,j)
                end do
                write(*,*)
            end do
        end do


        if( allocated(myArray) ) deallocate(myArray)

    end if

    call adios2_close( engine, ierr )
    call adios2_finalize(adios, ierr)
    call MPI_Finalize(ierr)

end program helloInsituMPIReader
