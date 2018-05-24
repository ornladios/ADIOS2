program TestRemove
    use small_test_data
    use adios2
    implicit none

    integer(kind=8), dimension(1) :: shape_dims, start_dims, count_dims
    integer :: inx, irank, isize, ierr, i

    ! low-level
    type(adios2_adios) :: adios
    type(adios2_io) :: ioWrite, ioRead
    type(adios2_variable), dimension(12) :: variables
    type(adios2_engine) :: bpWriter, bpReader


    ! Application variables
    inx = 10

    ! Variable dimensions
    shape_dims(1) = inx
    start_dims(1) = 0
    count_dims(1) = inx

    ! Create adios handler passing the communicator, debug mode and error flag
    call adios2_init(adios, adios2_debug_mode_on, ierr)

    !!!!!!!!!!!!!!!!!!!!!!!!! WRITER !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    ! Declare an IO process configuration inside adios
    call adios2_declare_io(ioWrite, adios, "ioWrite", ierr)

    ! Defines a variable to be written in bp format
    call adios2_define_variable(variables(1), ioWrite, "var_I8", &
        adios2_type_integer1, 1, &
        shape_dims, start_dims, count_dims, &
        adios2_constant_dims, ierr)

    call adios2_define_variable(variables(2), ioWrite, "var_I16", &
        adios2_type_integer2, 1, &
        shape_dims, start_dims, count_dims, &
        adios2_constant_dims, ierr)

    call adios2_define_variable(variables(3), ioWrite, "var_I32", &
        adios2_type_integer4, 1, &
        shape_dims, start_dims, count_dims, &
        adios2_constant_dims, ierr)

    call adios2_define_variable(variables(4), ioWrite, "var_I64", &
        adios2_type_integer8, 1, &
        shape_dims, start_dims, count_dims, &
        adios2_constant_dims, ierr)

    call adios2_define_variable(variables(5), ioWrite, "var_R32", &
        adios2_type_real, 1, &
        shape_dims, start_dims, count_dims, &
        adios2_constant_dims,  ierr)

    call adios2_define_variable(variables(6), ioWrite, "var_R64", &
        adios2_type_dp, 1, &
        shape_dims, start_dims, count_dims, &
        adios2_constant_dims,  ierr)

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

    ! remove piece
    call adios2_remove_variable(ioWrite, "gvar_R64", ierr)
    if (ierr /= 1) stop 'gvar_R64 not removed'

    call adios2_inquire_variable(variables(12), ioWrite, "gvar_R64", ierr)
    if (ierr == adios2_found) stop 'gvar_R64 found with inquire, not removed'

    ! remove all
    call adios2_remove_all_variables(ioWrite, ierr)

    call adios2_inquire_variable(variables(1), ioWrite, "var_I8", ierr)
    if (ierr == adios2_found) stop 'var_I8 found'

    call adios2_inquire_variable(variables(2), ioWrite, "var_I16", ierr)
    if (ierr == adios2_found) stop 'var_I16 found'

    call adios2_inquire_variable(variables(3), ioWrite, "var_I32", ierr)
    if (ierr == adios2_found) stop 'var_I32 found'

    call adios2_inquire_variable(variables(4), ioWrite, "var_I64", ierr)
    if (ierr == adios2_found) stop 'var_I64 found'

    call adios2_inquire_variable(variables(5), ioWrite, "var_R32", ierr)
    if (ierr == adios2_found) stop 'var_R32 found'

    call adios2_inquire_variable(variables(6), ioWrite, "var_R64", ierr)
    if (ierr == adios2_found) stop 'var_R64 found'

    call adios2_inquire_variable(variables(7), ioWrite, "gvar_I8", ierr)
    if (ierr == adios2_found) stop 'gvar_I8 found'

    call adios2_inquire_variable(variables(8), ioWrite, "gvar_I16", ierr)
    if (ierr == adios2_found) stop 'gvar_I16 found'

    call adios2_inquire_variable(variables(9), ioWrite, "gvar_I32", ierr)
    if (ierr == adios2_found) stop 'gvar_I32 found'

    call adios2_inquire_variable(variables(10), ioWrite, "gvar_I64", ierr)
    if (ierr == adios2_found) stop 'gvar_I64 found'

    call adios2_inquire_variable(variables(11), ioWrite, "gvar_R32", ierr)
    if (ierr == adios2_found) stop 'gvar_R32 found'

    call adios2_finalize(adios, ierr)

end program TestRemove
