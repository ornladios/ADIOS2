program TestBPWriteReadHeatMap3D
    use mpi
    use adios2

    implicit none

    integer(kind=8) :: adios
    integer(kind=8) :: ioPut, var_temperatures, bpWriter
    integer(kind=8) :: ioGet, var_temperaturesIn, bpReader
    !integer, dimension(:,:,:), allocatable :: temperatures, sel_temperatures
    real(kind=8), dimension(:,:,:), allocatable :: temperatures, sel_temperatures
    integer(kind=8), dimension(3) :: ishape, istart, icount
    integer(kind=8), dimension(3) :: sel_start, sel_count
    integer :: ierr, irank, isize, inx, iny, inz
    integer :: i, j, k, iglobal, value, ilinear, icounter

    call MPI_INIT(ierr)
    call MPI_COMM_RANK(MPI_COMM_WORLD, irank, ierr)
    call MPI_COMM_SIZE(MPI_COMM_WORLD, isize, ierr)

    inx = 10
    iny = 10
    inz = 10

    icount = (/ inx, iny, inz  /)
    istart = (/ 0,   0,   inz * irank  /)
    ishape = (/ inx, iny, inz * isize   /)

    allocate( temperatures( inx, iny, inz ) )
    temperatures = 1.0_8

    ! Start adios2 Writer
    call adios2_init( adios, MPI_COMM_WORLD, adios2_debug_mode_on, ierr )
    call adios2_declare_io( ioPut, adios, 'HeatMapWrite', ierr )

    call adios2_define_variable( var_temperatures, ioPut, 'temperatures', 3, &
                                 ishape, istart, icount, adios2_constant_dims, &
                                 temperatures, ierr )

    call adios2_open( bpWriter, ioPut, 'HeatMap3D_f.bp', adios2_mode_write, &
                      ierr )

    call adios2_put_sync( bpWriter, var_temperatures, temperatures, ierr )

    call adios2_close( bpWriter, ierr )


    if( allocated(temperatures) ) deallocate(temperatures)

    ! Start adios2 Reader in rank 0
    if( irank == 0 ) then

        call adios2_declare_io( ioGet, adios, 'HeatMapRead', ierr )

        call adios2_open( bpReader, ioGet, 'HeatMap3D_f.bp', &
                        & adios2_mode_read, MPI_COMM_SELF, ierr)

        call adios2_inquire_variable( var_temperaturesIn, ioGet, &
                                    & 'temperatures', ierr )

        print *, 'adios2_found', adios2_found

        sel_start = (/ 0, 0, 0 /)
        sel_count = (/ ishape(1), ishape(2), ishape(3) /)
        write(*,*) "Shapes: ", ishape(1), " ", ishape(2), " ", ishape(3)


        allocate( sel_temperatures( sel_count(1), sel_count(2), &
                                    & sel_count(3) ) )
        sel_temperatures = 0.0_8

        call adios2_set_selection( var_temperaturesIn, 3, sel_start, &
                                    & sel_count, ierr )

        call adios2_get_deferred( bpReader, var_temperaturesIn, &
                            & sel_temperatures, &
                            & ierr )

        call adios2_close( bpReader, ierr )

        do k=1,sel_count(3)
            do j=1,sel_count(2)
                do i=1,sel_count(1)
                    write(6,'(F3.1,A1)', advance="no") sel_temperatures(i,j,k), ' '
                end do
                write(*,*)
            end do
        end do

        write(*,'(A,3(I5,A),A,3(I5,A),A)') 'Temperature map selection  &
                    & [ start = (', (sel_start(i),',',i=1,3) , ') &
                    &  count =  (', (sel_count(i),',',i=1,3) , ') ]'

        !! Expect 1000.0 * isize
        print *, 'sum(sel_temperatures): expecting', 1000.0 * isize, &
                 'got:', sum(sel_temperatures)

        if( allocated(sel_temperatures) ) deallocate(sel_temperatures)


    end if

    call adios2_finalize(adios, ierr)
    call MPI_Finalize(ierr)

end program TestBPWriteReadHeatMap3D
