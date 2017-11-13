program helloBPReaderHeatMap3D
    use mpi
    use adios2

    implicit none

    integer(kind=8) :: adios, io, var_temperatures, engine
    integer, dimension(:,:,:), allocatable :: temperatures
    integer, dimension(3) :: ishape, istart, icount
    integer :: ierr, irank, isize, inx, iny, inz
    integer :: i, j, k, iglobal, value, ilinear

    call MPI_INIT(ierr)
    call MPI_COMM_RANK(MPI_COMM_WORLD, irank, ierr)
    call MPI_COMM_SIZE(MPI_COMM_WORLD, isize, ierr)

    inx = 10
    iny = 10
    inz = 10

    icount = (/         inx, iny, inz  /)
    istart = (/ irank * inx,   1,   1  /)
    ishape = (/ isize * inx, iny, inz  /)

    allocate( temperatures( inx, iny, inz ) )
    ! populate temperature
    do k=1, icount(3)
        do j=1, icount(2)
            do i=1, icount(1)
                iglobal = istart(1) + i
                value = k * ishape(2) * ishape(1) + j * ishape(1) + &
                &       iglobal
                temperatures(i,j,k) = value
            end do
        end do
    end do

    ! Start adios2 Writer
    call adios2_init( adios, MPI_COMM_WORLD, adios2_debug_mode_on, ierr )
    call adios2_declare_io( io, adios, "bpFileIO", ierr )

    call adios2_define_variable( var_temperatures, io, "temperatures", &
        & adios2_type_integer, 3, ishape, istart, icount, &
        & adios2_constant_dims_true, ierr )

    call adios2_open( engine, io, "HeatMap3D_f.bp", adios2_mode_write, ierr )

    call adios2_write( engine, var_temperatures, temperatures, ierr )

    call adios2_close( engine, ierr )


    if( allocated(temperatures) ) deallocate(temperatures)

    ! Start adios2 Reader



end program helloBPReaderHeatMap3D
