program TestBPMemorySpace
     use adios2
     implicit none

     integer(kind=8), dimension(1) :: shape_dims, start_dims, count_dims
     integer(kind=4) :: ierr, mem

     type(adios2_adios) :: adios
     type(adios2_variable) :: variable
     type(adios2_io) :: ioWrite

     shape_dims(1) = 10
     start_dims(1) = 0
     count_dims(1) = 10

     if( adios%valid .eqv. .true. ) stop 'Invalid adios default'
     if( variable%valid .eqv. .true. ) stop 'Invalid variables default'

     call adios2_init(adios, ierr)
     if( adios%valid .eqv. .false. ) stop 'Invalid adios2_init'

     call adios2_declare_io(ioWrite, adios, "ioWrite", ierr)
     if( ioWrite%valid .eqv. .false. ) stop 'Invalid adios2_declare_io'

     call adios2_set_engine(ioWrite, 'File', ierr)

     call adios2_define_variable(variable, ioWrite, "var_I32", &
                                 adios2_type_integer4, 1, &
                                 shape_dims, start_dims, count_dims, &
                                 adios2_constant_dims, ierr)

     ! check that the default execution space is Detect
     call adios2_get_memory_space(mem, variable, ierr)
     if (mem /= adios2_memory_space_detect) stop 'Invalid adios2_memory_space'

     ! check that the execution space is updated to Host
     call adios2_set_memory_space(variable, adios2_memory_space_host, ierr)
     call adios2_get_memory_space(mem, variable, ierr)
     if (mem /= adios2_memory_space_host) stop 'Invalid adios2_memory_space'

end program TestBPMemorySpace
