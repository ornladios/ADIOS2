! SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
!
! SPDX-License-Identifier: Apache-2.0

program TestBPMemorySpace
     use adios2
     implicit none

     integer(kind=8), dimension(1) :: shape_dims, start_dims, count_dims
     integer(kind=8), dimension(2) :: shape2, start2
     integer(kind=4) :: ierr, mem, layout
     integer(kind=4), dimension(3, 2) :: data2

     type(adios2_adios) :: adios
     type(adios2_variable) :: variable, var_col, var_row
     type(adios2_io) :: ioWrite, ioLayout, ioRead
     type(adios2_engine) :: engine

     shape_dims(1) = 10
     start_dims(1) = 0
     count_dims(1) = 10

     if( adios%valid .eqv. .true. ) then
        write(*,*) 'Invalid adios default'
        stop 1
     end if
     if( variable%valid .eqv. .true. ) then
        write(*,*) 'Invalid variables default'
        stop 1
     end if

     call adios2_init(adios, ierr)
     if( adios%valid .eqv. .false. ) then
        write(*,*) 'Invalid adios2_init'
        stop 1
     end if

     call adios2_declare_io(ioWrite, adios, "ioWrite", ierr)
     if( ioWrite%valid .eqv. .false. ) then
        write(*,*) 'Invalid adios2_declare_io'
        stop 1
     end if

     call adios2_set_engine(ioWrite, 'File', ierr)

     call adios2_define_variable(variable, ioWrite, "var_I32", &
                                 adios2_type_integer4, 1, &
                                 shape_dims, start_dims, count_dims, &
                                 adios2_constant_dims, ierr)

     ! check that the default execution space is Detect
     call adios2_get_memory_space(mem, variable, ierr)
     if (mem /= adios2_memory_space_detect) then
        write(*,*) 'Invalid default adios2_memory_space'
        stop 1
     end if

     ! check that the execution space is updated to Host
     call adios2_set_memory_space(variable, adios2_memory_space_host, ierr)
     call adios2_get_memory_space(mem, variable, ierr)
     if (mem /= adios2_memory_space_host) then
        write(*,*) 'Invalid set adios2_memory_space'
        stop 1
     end if

     ! a reader learns the layout each array was written from: Fortran's own
     ! column-major order, or row-major when the buffer layout says so
     shape2 = (/ 3, 2 /)
     start2 = (/ 0, 0 /)
     data2 = reshape((/ 0, 1, 2, 3, 4, 5 /), (/ 3, 2 /))
     call adios2_declare_io(ioLayout, adios, "ioLayout", ierr)
     call adios2_define_variable(var_col, ioLayout, "var_col", adios2_type_integer4, 2, &
                                 shape2, start2, shape2, adios2_constant_dims, ierr)
     call adios2_define_variable(var_row, ioLayout, "var_row", adios2_type_integer4, 2, &
                                 shape2, start2, shape2, adios2_constant_dims, ierr)
     call adios2_set_array_layout(var_row, adios2_arrayordering_rowmajor, ierr)
     call adios2_open(engine, ioLayout, "FortranArrayLayout.bp", adios2_mode_write, ierr)
     call adios2_put(engine, var_col, data2, adios2_mode_sync, ierr)
     call adios2_put(engine, var_row, data2, adios2_mode_sync, ierr)
     call adios2_close(engine, ierr)

     call adios2_declare_io(ioRead, adios, "ioRead", ierr)
     call adios2_open(engine, ioRead, "FortranArrayLayout.bp", adios2_mode_readRandomAccess, ierr)
     call adios2_inquire_variable(var_col, ioRead, "var_col", ierr)
     call adios2_writer_array_layout(layout, var_col, ierr)
     if (ierr /= 0 .or. layout /= adios2_arrayordering_columnmajor) then
        write(*,*) 'Wrong writer layout for var_col'
        stop 1
     end if
     call adios2_inquire_variable(var_row, ioRead, "var_row", ierr)
     call adios2_writer_array_layout(layout, var_row, ierr)
     if (ierr /= 0 .or. layout /= adios2_arrayordering_rowmajor) then
        write(*,*) 'Wrong writer layout for var_row'
        stop 1
     end if
     call adios2_close(engine, ierr)

end program TestBPMemorySpace
