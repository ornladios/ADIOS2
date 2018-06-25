program TestBPReadGlobalsByName
    use mpi
    use adios2
    implicit none

    integer(kind=8), dimension(2) :: sel_start, sel_count
    real, dimension(:,:), allocatable :: data
    integer :: i, j, inx, iny, irank, isize, ierr
    integer :: diag_1d_isp=999,diag_1d_nsp=999  ! same as ptl_isp/nsp
    real(kind=8) :: sml_inpsi=999.0D0   , sml_outpsi=999.0D0  !! Inner and outer boundary for initial loading.


    ! adios2 handlers
    type(adios2_adios):: adios
    type(adios2_io):: ioWrite, ioRead
    type(adios2_variable):: var
    type(adios2_engine):: writer, reader

    ! Launch MPI
    call MPI_Init(ierr)
    call MPI_Comm_rank(MPI_COMM_WORLD, irank, ierr)
    call MPI_Comm_size(MPI_COMM_WORLD, isize, ierr)

    ! Start adios2
    call adios2_init(adios, MPI_COMM_SELF, adios2_debug_mode_on, ierr)

    ! writer
    call adios2_declare_io(ioWrite, adios, "FWriter", ierr)

    call adios2_define_variable(var, ioWrite, "diag_1d_nsp", &
                                adios2_type_integer4, ierr)
    call adios2_define_variable(var, ioWrite, "sml_outpsi", adios2_type_dp, &
                                ierr)

    call adios2_open(writer, ioWrite, "xgc.f0analysis.static.bp", &
                     adios2_mode_write, ierr)
    call adios2_put(writer, "diag_1d_nsp", 1, ierr)
    call adios2_put(writer, "sml_outpsi", 0.295477_8, ierr)
    call adios2_close(writer, ierr)

    ! reader
    call adios2_declare_io(ioRead, adios, "FReader", ierr)
    call adios2_open(reader, ioRead, "xgc.f0analysis.static.bp", adios2_mode_read, ierr)
    call adios2_get(reader, "diag_1d_nsp", diag_1d_nsp, ierr)
    call adios2_get(reader, "sml_outpsi", sml_outpsi, ierr)
    call adios2_close(reader, ierr)

    call adios2_finalize(adios, ierr)

    !! Expect 1 and 0.295477 for diag_1d_nsp and sml_outpsi
    if( diag_1d_nsp /= 1 ) then
        print *, irank, 'diag_1d_nsp', diag_1d_nsp
        stop 'diag_1d_nsp is not 1'
    end if

    if( sml_outpsi /= 0.295477_8 ) then
        print *, irank, 'sml_outpsi', sml_outpsi
        stop 'sml_outpsi is not 0.295477'
    end if

    call MPI_Finalize(ierr)

end program TestBPReadGlobalsByName
