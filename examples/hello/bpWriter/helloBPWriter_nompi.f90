program helloBPWriter

    use adios2

    implicit none

    integer :: inx, i, ierr
    integer(kind=8) :: adios, io, var1, engine1
    real, dimension(:), allocatable :: myArray
    integer, dimension(1) :: shape_dims, start_dims, count_dims

    !application variables
    inx = 10
    allocate( myArray(inx) )

    do i=1,inx
        myArray(i) = i-1
    end do

    shape_dims(1) = inx
    start_dims(1) = 0
    count_dims(1) = inx

    ! Initialize adios handler
    call adios2_init(adios, adios2_debug_mode_on, ierr)

    ! Declare an IO configuration inside adios
    call adios2_declare_io(io, adios, "bpIO", ierr)

    ! Defines variable metadata to be written in bp file
    call adios2_define_variable(var1, io, "myArray", adios2_type_real, 1, &
        & shape_dims, start_dims, count_dims, adios2_constant_dims_true, ierr)

    ! Open myVector_f.bp in write mode
    call adios2_open(engine1, io, "myVector_f.bp", adios2_mode_write, ierr)

    ! Write myArray contents to bp buffer, based on var1 metadata
    call adios2_write(engine1, var1, myArray, ierr)

    ! Closes engine1 and deallocates it, becomes unreachable
    call adios2_close(engine1, ierr)

    ! Deallocates adios and calls its destructor
    call adios2_finalize(adios, ierr)

    deallocate(myArray)

end program helloBPWriter
