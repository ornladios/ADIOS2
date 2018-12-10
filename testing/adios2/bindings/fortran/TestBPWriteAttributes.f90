program TestBPWriteAttributes
    use small_test_data
    use mpi
    use adios2
    implicit none

    type(adios2_adios) :: adios
    type(adios2_io) :: ioWrite, ioRead
    type(adios2_engine) :: bpWriter, bpReader
    type(adios2_attribute), dimension(14) :: attributes

    integer :: ierr, i
    integer(kind=2):: i16_value




    ! Launch MPI
    call MPI_Init(ierr)

    ! Create adios handler passing the communicator, debug mode and error flag
    call adios2_init(adios, MPI_COMM_WORLD, adios2_debug_mode_on, ierr)

    !!!!!!!!!!!!!!!!!!!!!!!!! WRITER !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    ! Declare an IO process configuration inside adios
    call adios2_declare_io(ioWrite, adios, "ioWrite", ierr)

    do i=1,14
        if( attributes(i)%valid .eqv. .true. ) stop 'Invalid attribute default'
    end do

    ! single value
    call adios2_define_attribute(attributes(1), ioWrite, 'att_String', &
                                 'ADIOS2 String attribute', ierr)

    call adios2_define_attribute(attributes(2), ioWrite, 'att_i8', &
                                 data_I8(1), ierr)

    call adios2_define_attribute(attributes(3), ioWrite, 'att_i16', &
                                 data_I16(1), ierr)

    call adios2_define_attribute(attributes(4), ioWrite, 'att_i32', &
                                 data_I32(1), ierr)

    call adios2_define_attribute(attributes(5), ioWrite, 'att_i64', &
                                 data_I64(1), ierr)

    call adios2_define_attribute(attributes(6), ioWrite, 'att_r32', &
                                 data_R32(1), ierr)

    call adios2_define_attribute(attributes(7), ioWrite, 'att_r64', &
                                 data_R64(1), ierr)

    ! arrays
    call adios2_define_attribute(attributes(8), ioWrite, 'att_Strings_array', &
                                 data_Strings, 3, ierr)

    call adios2_define_attribute(attributes(9), ioWrite, 'att_i8_array', &
                                 data_I8, 3, ierr)

    call adios2_define_attribute(attributes(10), ioWrite, 'att_i16_array', &
                                 data_I16, 3, ierr)

    call adios2_define_attribute(attributes(11), ioWrite, 'att_i32_array', &
                                 data_I32, 3, ierr)

    call adios2_define_attribute(attributes(12), ioWrite, 'att_i64_array', &
                                 data_I64, 3, ierr)

    call adios2_define_attribute(attributes(13), ioWrite, 'att_r32_array', &
                                 data_R32, 3, ierr)

    call adios2_define_attribute(attributes(14), ioWrite, 'att_r64_array', &
                                 data_R64, 3, ierr)

    do i=1,14
        if( attributes(i)%valid .eqv. .false. ) stop 'Invalid adios2_define_attribute'
    end do

    call adios2_open(bpWriter, ioWrite, "fattr_types.bp", adios2_mode_write, &
                     ierr)

    call adios2_close(bpWriter, ierr)

    call MPI_Barrier(MPI_COMM_WORLD, ierr)

    !!!!!!!!!!!!!!!!!!!!!!!!! READER !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    call adios2_declare_io(ioRead, adios, "ioRead", ierr)

    call adios2_open(bpReader, ioRead, 'fattr_types.bp', adios2_mode_read, ierr)

    call adios2_inquire_attribute(attributes(3), ioRead, 'att_i16', ierr)

    if(attributes(3)%valid .eqv. .false.) stop 'attribute i16 not found'
    if(attributes(3)%type /= adios2_type_integer2) stop 'attribute i16 wrong type'
    if(attributes(3)%length /= 1) stop 'attribute i16 lenght is not 1'
    if(attributes(3)%is_value .eqv. .false.) stop 'attribute i16 must be value'
    call adios2_attribute_data( i16_value, attributes(3), ierr)
    if( i16_value /=  data_I16(1) ) stop 'attribute i16 data error'



    call adios2_close(bpReader, ierr)





    call adios2_finalize(adios, ierr)

    call MPI_Finalize(ierr)

end program TestBPWriteAttributes
