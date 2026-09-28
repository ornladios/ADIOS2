! SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
!
! SPDX-License-Identifier: Apache-2.0

program TestBPWriteReadHeatMap6D
  use mpi
  use adios2

  implicit none

  type(adios2_adios) :: adios
  type(adios2_io) :: ioPut, ioGet
  type(adios2_engine) :: bpWriter, bpReader
  type(adios2_variable), dimension(6) :: var_temperatures, var_temperaturesIn

  integer(kind=1), dimension(:, :, :, :, :, :), allocatable :: temperatures_i1, &
                                                               sel_temperatures_i1

  integer(kind=2), dimension(:, :, :, :, :, :), allocatable :: temperatures_i2, &
                                                               sel_temperatures_i2

  integer(kind=4), dimension(:, :, :, :, :, :), allocatable :: temperatures_i4, &
                                                               sel_temperatures_i4

  integer(kind=8), dimension(:, :, :, :, :, :), allocatable :: temperatures_i8, &
                                                               sel_temperatures_i8

  real(kind=4), dimension(:, :, :, :, :, :), allocatable :: temperatures_r4, &
                                                            sel_temperatures_r4

  real(kind=8), dimension(:, :, :, :, :, :), allocatable :: temperatures_r8, &
                                                            sel_temperatures_r8

  integer(kind=8), dimension(6) :: ishape, istart, icount
  integer(kind=8), dimension(6) :: sel_start, sel_count
  integer :: ierr, irank, isize, step_status
  integer :: in1, in2, in3, in4, in5, in6
  integer :: i1, i2, i3, i4, i5, i6
  integer(kind=8) :: index_value

  call MPI_INIT(ierr)
  call MPI_COMM_RANK(MPI_COMM_WORLD, irank, ierr)
  call MPI_COMM_SIZE(MPI_COMM_WORLD, isize, ierr)

  in1 = 10
  in2 = 10
  in3 = 10
  in4 = 10
  in5 = 10
  in6 = 10

  icount = (/in1, in2, in3, in4, in5, in6/)
  istart = (/0, 0, 0, 0, 0, in6*irank/)
  ishape = (/in1, in2, in3, in4, in5, in6*isize/)

  allocate (temperatures_i1(in1, in2, in3, in4, in5, in6))
  allocate (temperatures_i2(in1, in2, in3, in4, in5, in6))
  allocate (temperatures_i4(in1, in2, in3, in4, in5, in6))
  allocate (temperatures_i8(in1, in2, in3, in4, in5, in6))
  allocate (temperatures_r4(in1, in2, in3, in4, in5, in6))
  allocate (temperatures_r8(in1, in2, in3, in4, in5, in6))

  do i6 = 1, in6
    do i5 = 1, in5
      do i4 = 1, in4
        do i3 = 1, in3
          do i2 = 1, in2
            do i1 = 1, in1
              index_value = HeatMapIndex(i1, i2, i3, i4, i5, i6, irank)
              temperatures_i1(i1, i2, i3, i4, i5, i6) = ExpectedInteger1(index_value)
              temperatures_i2(i1, i2, i3, i4, i5, i6) = ExpectedInteger2(index_value)
              temperatures_i4(i1, i2, i3, i4, i5, i6) = ExpectedInteger4(index_value)
              temperatures_i8(i1, i2, i3, i4, i5, i6) = ExpectedInteger8(index_value)
              temperatures_r4(i1, i2, i3, i4, i5, i6) = ExpectedReal4(index_value)
              temperatures_r8(i1, i2, i3, i4, i5, i6) = ExpectedReal8(index_value)
            end do
          end do
        end do
      end do
    end do
  end do

  ! Start adios2 Writer
  call adios2_init(adios, MPI_COMM_WORLD, ierr)
  call adios2_declare_io(ioPut, adios, 'HeatMapWrite', ierr)

  call adios2_define_variable(var_temperatures(1), ioPut, &
                              'temperatures_i1', adios2_type_integer1, &
                              6, ishape, istart, icount, &
                              adios2_constant_dims, ierr)

  call adios2_define_variable(var_temperatures(2), ioPut, &
                              'temperatures_i2', adios2_type_integer2, &
                              6, ishape, istart, icount, &
                              adios2_constant_dims, ierr)

  call adios2_define_variable(var_temperatures(3), ioPut, &
                              'temperatures_i4', adios2_type_integer4, &
                              6, ishape, istart, icount, &
                              adios2_constant_dims, ierr)

  call adios2_define_variable(var_temperatures(4), ioPut, &
                              'temperatures_i8', adios2_type_integer8, &
                              6, ishape, istart, icount, &
                              adios2_constant_dims, ierr)

  call adios2_define_variable(var_temperatures(5), ioPut, &
                              'temperatures_r4', adios2_type_real, &
                              6, ishape, istart, icount, &
                              adios2_constant_dims, ierr)

  call adios2_define_variable(var_temperatures(6), ioPut, &
                              'temperatures_r8', adios2_type_dp, &
                              6, ishape, istart, icount, &
                              adios2_constant_dims, ierr)

  call adios2_open(bpWriter, ioPut, 'HeatMap6D_f.bp', adios2_mode_write, &
                   ierr)

  call adios2_put(bpWriter, var_temperatures(1), temperatures_i1, ierr)
  call adios2_put(bpWriter, var_temperatures(2), temperatures_i2, ierr)
  call adios2_put(bpWriter, var_temperatures(3), temperatures_i4, ierr)
  call adios2_put(bpWriter, var_temperatures(4), temperatures_i8, ierr)
  call adios2_put(bpWriter, var_temperatures(5), temperatures_r4, ierr)
  call adios2_put(bpWriter, var_temperatures(6), temperatures_r8, ierr)

  call adios2_close(bpWriter, ierr)

  if (allocated(temperatures_i1)) deallocate (temperatures_i1)
  if (allocated(temperatures_i2)) deallocate (temperatures_i2)
  if (allocated(temperatures_i4)) deallocate (temperatures_i4)
  if (allocated(temperatures_i8)) deallocate (temperatures_i8)
  if (allocated(temperatures_r4)) deallocate (temperatures_r4)
  if (allocated(temperatures_r8)) deallocate (temperatures_r8)

  ! Each MPI rank reads the same hyperslab it wrote.
  call adios2_declare_io(ioGet, adios, 'HeatMapRead', ierr)
  call adios2_open(bpReader, ioGet, 'HeatMap6D_f.bp', adios2_mode_read, ierr)

  call adios2_begin_step(bpReader, adios2_step_mode_read, -1., step_status, ierr)

  call adios2_inquire_variable(var_temperaturesIn(1), ioGet, &
                               'temperatures_i1', ierr)
  call adios2_inquire_variable(var_temperaturesIn(2), ioGet, &
                               'temperatures_i2', ierr)
  call adios2_inquire_variable(var_temperaturesIn(3), ioGet, &
                               'temperatures_i4', ierr)
  call adios2_inquire_variable(var_temperaturesIn(4), ioGet, &
                               'temperatures_i8', ierr)
  call adios2_inquire_variable(var_temperaturesIn(5), ioGet, &
                               'temperatures_r4', ierr)
  call adios2_inquire_variable(var_temperaturesIn(6), ioGet, &
                               'temperatures_r8', ierr)

  sel_start = istart
  sel_count = icount

  allocate (sel_temperatures_i1(in1, in2, in3, in4, in5, in6))
  allocate (sel_temperatures_i2(in1, in2, in3, in4, in5, in6))
  allocate (sel_temperatures_i4(in1, in2, in3, in4, in5, in6))
  allocate (sel_temperatures_i8(in1, in2, in3, in4, in5, in6))
  allocate (sel_temperatures_r4(in1, in2, in3, in4, in5, in6))
  allocate (sel_temperatures_r8(in1, in2, in3, in4, in5, in6))

  call adios2_set_selection(var_temperaturesIn(1), 6, sel_start, sel_count, ierr)
  call adios2_set_selection(var_temperaturesIn(2), 6, sel_start, sel_count, ierr)
  call adios2_set_selection(var_temperaturesIn(3), 6, sel_start, sel_count, ierr)
  call adios2_set_selection(var_temperaturesIn(4), 6, sel_start, sel_count, ierr)
  call adios2_set_selection(var_temperaturesIn(5), 6, sel_start, sel_count, ierr)
  call adios2_set_selection(var_temperaturesIn(6), 6, sel_start, sel_count, ierr)

  call adios2_get(bpReader, var_temperaturesIn(1), sel_temperatures_i1, ierr)
  call adios2_get(bpReader, var_temperaturesIn(2), sel_temperatures_i2, ierr)
  call adios2_get(bpReader, var_temperaturesIn(3), sel_temperatures_i4, ierr)
  call adios2_get(bpReader, var_temperaturesIn(4), sel_temperatures_i8, ierr)
  call adios2_get(bpReader, var_temperaturesIn(5), sel_temperatures_r4, ierr)
  call adios2_get(bpReader, var_temperaturesIn(6), sel_temperatures_r8, ierr)

  call adios2_end_step(bpReader, ierr)
  call adios2_close(bpReader, ierr)

  do i6 = 1, in6
    do i5 = 1, in5
      do i4 = 1, in4
        do i3 = 1, in3
          do i2 = 1, in2
            do i1 = 1, in1
              index_value = HeatMapIndex(i1, i2, i3, i4, i5, i6, irank)
              if (sel_temperatures_i1(i1, i2, i3, i4, i5, i6) /= &
                  ExpectedInteger1(index_value)) then
                write(*,*) 'Test failed integer*1 at rank/coordinate ', irank, &
                           i1, i2, i3, i4, i5, i6
                stop 1
              end if
              if (sel_temperatures_i2(i1, i2, i3, i4, i5, i6) /= &
                  ExpectedInteger2(index_value)) then
                write(*,*) 'Test failed integer*2 at rank/coordinate ', irank, &
                           i1, i2, i3, i4, i5, i6
                stop 1
              end if
              if (sel_temperatures_i4(i1, i2, i3, i4, i5, i6) /= &
                  ExpectedInteger4(index_value)) then
                write(*,*) 'Test failed integer*4 at rank/coordinate ', irank, &
                           i1, i2, i3, i4, i5, i6
                stop 1
              end if
              if (sel_temperatures_i8(i1, i2, i3, i4, i5, i6) /= &
                  ExpectedInteger8(index_value)) then
                write(*,*) 'Test failed integer*8 at rank/coordinate ', irank, &
                           i1, i2, i3, i4, i5, i6
                stop 1
              end if
              if (sel_temperatures_r4(i1, i2, i3, i4, i5, i6) /= &
                  ExpectedReal4(index_value)) then
                write(*,*) 'Test failed real*4 at rank/coordinate ', irank, &
                           i1, i2, i3, i4, i5, i6
                stop 1
              end if
              if (sel_temperatures_r8(i1, i2, i3, i4, i5, i6) /= &
                  ExpectedReal8(index_value)) then
                write(*,*) 'Test failed real*8 at rank/coordinate ', irank, &
                           i1, i2, i3, i4, i5, i6
                stop 1
              end if
            end do
          end do
        end do
      end do
    end do
  end do

  if (allocated(sel_temperatures_i1)) deallocate (sel_temperatures_i1)
  if (allocated(sel_temperatures_i2)) deallocate (sel_temperatures_i2)
  if (allocated(sel_temperatures_i4)) deallocate (sel_temperatures_i4)
  if (allocated(sel_temperatures_i8)) deallocate (sel_temperatures_i8)
  if (allocated(sel_temperatures_r4)) deallocate (sel_temperatures_r4)
  if (allocated(sel_temperatures_r8)) deallocate (sel_temperatures_r8)

  call adios2_finalize(adios, ierr)
  call MPI_Finalize(ierr)

contains

  pure integer(kind=8) function HeatMapIndex(i1, i2, i3, i4, i5, i6, rank) result(index)
    integer, intent(in) :: i1, i2, i3, i4, i5, i6, rank
    integer(kind=8) :: global_i6

    global_i6 = 10_8 * int(rank, kind=8) + int(i6 - 1, kind=8)
    index = int(i1 - 1, kind=8) + 10_8 * (int(i2 - 1, kind=8) + 10_8 * &
            (int(i3 - 1, kind=8) + 10_8 * (int(i4 - 1, kind=8) + 10_8 * &
            (int(i5 - 1, kind=8) + 10_8 * global_i6))))
  end function HeatMapIndex

  pure integer(kind=1) function ExpectedInteger1(index) result(value)
    integer(kind=8), intent(in) :: index
    value = int(modulo(index, 251_8) - 125_8, kind=1)
  end function ExpectedInteger1

  pure integer(kind=2) function ExpectedInteger2(index) result(value)
    integer(kind=8), intent(in) :: index
    value = int(modulo(index, 32749_8) - 16374_8, kind=2)
  end function ExpectedInteger2

  pure integer(kind=4) function ExpectedInteger4(index) result(value)
    integer(kind=8), intent(in) :: index
    value = int(index + 1_8, kind=4)
  end function ExpectedInteger4

  pure integer(kind=8) function ExpectedInteger8(index) result(value)
    integer(kind=8), intent(in) :: index
    value = index + 1_8
  end function ExpectedInteger8

  pure real(kind=4) function ExpectedReal4(index) result(value)
    integer(kind=8), intent(in) :: index
    value = real(modulo(index, 16777213_8), kind=4)
  end function ExpectedReal4

  pure real(kind=8) function ExpectedReal8(index) result(value)
    integer(kind=8), intent(in) :: index
    value = real(index + 1_8, kind=8)
  end function ExpectedReal8

end program TestBPWriteReadHeatMap6D
