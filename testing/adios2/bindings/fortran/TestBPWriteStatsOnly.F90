program TestBPWriteStatsOnly
  use adios2

  implicit none

  type(adios2_adios) :: adios
  type(adios2_io) :: ioPut, ioGet
  type(adios2_engine) :: bpWriter, bpReader
  type(adios2_variable) :: var_temperatures, var_temperaturesIn

  integer(kind=4), dimension(:, :), allocatable :: temperatures_i4, &
                                                   sel_temperatures_i4

  integer(kind=8), dimension(2) :: ishape, istart, icount
  integer(kind=4) :: ierr
  integer :: step_status
  integer :: in1, in2

  in1 = 5
  in2 = 10

  icount = (/in1, in2/)
  istart = (/0, 0/)
  ishape = (/in1, in2/)

  allocate (temperatures_i4(in1, in2))
  temperatures_i4 = 1

  ! Start adios2 Writer
  call adios2_init(adios, ierr)
  call adios2_declare_io(ioPut, adios, 'TestWrite', ierr)

  call adios2_define_variable(var_temperatures, ioPut, &
                              'temperatures_i4', adios2_type_integer4, &
                              2, ishape, istart, icount, &
                              adios2_constant_dims, ierr)
  call adios2_store_stats_only(var_temperatures, .true., ierr)

  call adios2_open(bpWriter, ioPut, 'TestBPWriteStatsOnly_f.bp', adios2_mode_write, &
                   ierr)
  call adios2_put(bpWriter, var_temperatures, temperatures_i4, ierr)
  call adios2_close(bpWriter, ierr)
  if (allocated(temperatures_i4)) deallocate (temperatures_i4)

  call adios2_declare_io(ioGet, adios, 'TestRead', ierr)
  call adios2_open(bpReader, ioGet, 'TestBPWriteStatsOnly_f.bp', &
                   adios2_mode_read, ierr)

  call adios2_begin_step(bpReader, adios2_step_mode_read, -1., &
                         step_status, ierr)
  call adios2_inquire_variable(var_temperaturesIn, ioGet, &
                               'temperatures_i4', ierr)

  allocate (sel_temperatures_i4(ishape(1), ishape(2)))
  sel_temperatures_i4 = 0

  call adios2_get(bpReader, var_temperaturesIn, sel_temperatures_i4, adios2_mode_sync, ierr)
  if (ierr == 0) then
    write(*,*) 'adios2_get for a variable storing only stats returned without error'
    if (allocated(sel_temperatures_i4)) deallocate (sel_temperatures_i4)
    call adios2_close(bpReader, ierr)
    stop 1
  end if
  if (allocated(sel_temperatures_i4)) deallocate (sel_temperatures_i4)
  call adios2_close(bpReader, ierr)

end program TestBPWriteStatsOnly
