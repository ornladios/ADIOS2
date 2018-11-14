program TestBPWriteMemorySelectionRead2D
  use mpi
  use adios2

  implicit none

  type(adios2_adios) :: adios
  type(adios2_io) :: ioPut, ioGet
  type(adios2_engine) :: bpWriter, bpReader
  type(adios2_variable), dimension(6) :: vars, vars_in

  integer(kind=1), dimension(:, :), allocatable :: data_i1, in_data_i1
  integer(kind=2), dimension(:, :), allocatable :: data_i2, in_data_i2
  integer(kind=4), dimension(:, :), allocatable :: data_i4, in_data_i4
  integer(kind=8), dimension(:, :), allocatable :: data_i8, in_data_i8
  real(kind=4), dimension(:, :), allocatable :: data_r4, in_data_r4
  real(kind=8), dimension(:, :), allocatable :: data_r8, in_data_r8

  integer(kind=8), dimension(2) :: ishape, istart, icount
  integer(kind=8), dimension(2) :: memory_start, memory_count
  integer :: ierr, irank, isize, nx, ny, nsteps, s, step_status
  integer(kind=8) :: ghost_x, ghost_y, i, j, current_step

  call MPI_INIT(ierr)
  call MPI_COMM_RANK(MPI_COMM_WORLD, irank, ierr)
  call MPI_COMM_SIZE(MPI_COMM_WORLD, isize, ierr)

  nsteps = 3

  nx = 10 ! fastest
  ny = 20

  ghost_x = 2
  ghost_y = 4

  allocate (data_i1(nx + 2* ghost_x, ny + 2* ghost_y))
  allocate (data_i2(nx + 2* ghost_x, ny + 2* ghost_y))
  allocate (data_i4(nx + 2* ghost_x, ny + 2* ghost_y))
  allocate (data_i8(nx + 2* ghost_x, ny + 2* ghost_y))
  allocate (data_r4(nx + 2* ghost_x, ny + 2* ghost_y))
  allocate (data_r8(nx + 2* ghost_x, ny + 2* ghost_y))

  data_i1 = -1
  data_i2 = -1
  data_i4 = -1
  data_i8 = -1_8
  data_r4 = -1.0
  data_r8 = -1.0_8

  call adios2_init(adios, MPI_COMM_WORLD, adios2_debug_mode_on, ierr)

  ! Writer
  call adios2_declare_io(ioPut, adios, 'MemSelWriter', ierr)

  ishape = (/nx, isize*ny/)
  istart = (/0,  irank*ny/)
  icount = (/nx, ny/)

  memory_start = (/ ghost_x, ghost_y /)
  memory_count = (/ nx + 2* ghost_x, ny + 2* ghost_y /)

  call adios2_define_variable(vars(1), ioPut, 'var_i1', adios2_type_integer1, &
                              2, ishape, istart, icount, &
                              adios2_constant_dims, ierr)

  call adios2_define_variable(vars(2), ioPut, 'var_i2', adios2_type_integer2, &
                              2, ishape, istart, icount, &
                              adios2_constant_dims, ierr)

  call adios2_define_variable(vars(3), ioPut, 'var_i4', adios2_type_integer4, &
                              2, ishape, istart, icount, &
                              adios2_constant_dims, ierr)

  call adios2_define_variable(vars(4), ioPut, 'var_i8', adios2_type_integer8, &
                              2, ishape, istart, icount, &
                              adios2_constant_dims, ierr)

  call adios2_define_variable(vars(5), ioPut, 'var_r4', adios2_type_real, &
                              2, ishape, istart, icount, &
                              adios2_constant_dims, ierr)

  call adios2_define_variable(vars(6), ioPut, 'var_r8', adios2_type_dp, &
                              2, ishape, istart, icount, &
                              adios2_constant_dims, ierr)


  call adios2_set_memory_selection(vars(1), 2, memory_start, memory_count, ierr)
  call adios2_set_memory_selection(vars(2), 2, memory_start, memory_count, ierr)
  call adios2_set_memory_selection(vars(3), 2, memory_start, memory_count, ierr)
  call adios2_set_memory_selection(vars(4), 2, memory_start, memory_count, ierr)
  call adios2_set_memory_selection(vars(5), 2, memory_start, memory_count, ierr)
  call adios2_set_memory_selection(vars(6), 2, memory_start, memory_count, ierr)

  call adios2_open(bpWriter, ioPut, 'MemSel2D_f.bp', adios2_mode_write, ierr)

  do s=0,nsteps-1
    data_i1(ghost_x+1:nx+ghost_x+1,ghost_y+1:ny+ghost_y+1) = s
    data_i2(ghost_x+1:nx+ghost_x+1,ghost_y+1:ny+ghost_y+1) = s
    data_i4(ghost_x+1:nx+ghost_x+1,ghost_y+1:ny+ghost_y+1) = s
    data_i8(ghost_x+1:nx+ghost_x+1,ghost_y+1:ny+ghost_y+1) = s
    data_r4(ghost_x+1:nx+ghost_x+1,ghost_y+1:ny+ghost_y+1) = s
    data_r8(ghost_x+1:nx+ghost_x+1,ghost_y+1:ny+ghost_y+1) = s

    call adios2_begin_step(bpWriter, ierr)
    call adios2_put(bpWriter, vars(1), data_i1, ierr)
    call adios2_put(bpWriter, vars(2), data_i2, ierr)
    call adios2_put(bpWriter, vars(3), data_i4, ierr)
    call adios2_put(bpWriter, vars(4), data_i8, ierr)
    call adios2_put(bpWriter, vars(5), data_r4, ierr)
    call adios2_put(bpWriter, vars(6), data_r8, ierr)
    call adios2_end_step(bpWriter, ierr)

  end do

  call adios2_close(bpWriter, ierr)

  deallocate (data_i1)
  deallocate (data_i2)
  deallocate (data_i4)
  deallocate (data_i8)
  deallocate (data_r4)
  deallocate (data_r8)

  call MPI_Barrier(MPI_COMM_WORLD, ierr)

  ! reader goes here
  call adios2_declare_io(ioGet, adios, 'MemSelReader', ierr)

  call adios2_open(bpReader, ioGet, 'MemSel2D_f.bp', adios2_mode_read, ierr)

  allocate (in_data_i1(nx, isize*ny))
  allocate (in_data_i2(nx, isize*ny))
  allocate (in_data_i4(nx, isize*ny))
  allocate (in_data_i8(nx, isize*ny))
  allocate (in_data_r4(nx, isize*ny))
  allocate (in_data_r8(nx, isize*ny))

  do

    call adios2_begin_step(bpReader, adios2_step_mode_next_available, 0., &
                           step_status, ierr)

    if(step_status == adios2_step_status_end_of_stream) exit

    call adios2_get(bpReader, 'var_i1', in_data_i1, ierr)
    call adios2_get(bpReader, 'var_i2', in_data_i2, ierr)
    call adios2_get(bpReader, 'var_i4', in_data_i4, ierr)
    call adios2_get(bpReader, 'var_i8', in_data_i8, ierr)
    call adios2_get(bpReader, 'var_r4', in_data_r4, ierr)
    call adios2_get(bpReader, 'var_r8', in_data_r8, ierr)
    call adios2_end_step(bpReader, ierr)

    call adios2_current_step(current_step, bpReader, ierr)

    do j=1,isize*ny
      do i=1,nx
          if(in_data_i1(i,j) /= current_step) stop 'i1 read error'
          if(in_data_i2(i,j) /= current_step) stop 'i2 read error'
          if(in_data_i4(i,j) /= current_step) stop 'i4 read error'
          if(in_data_i8(i,j) /= current_step) stop 'i8 read rerror'
          if(in_data_r4(i,j) /= current_step) stop 'r4 read error'
          if(in_data_r8(i,j) /= current_step) stop 'r8 read rerror'
      end do
    end do

  end do

  call adios2_close(bpReader, ierr)

  deallocate (in_data_i1)
  deallocate (in_data_i2)
  deallocate (in_data_i4)
  deallocate (in_data_i8)
  deallocate (in_data_r4)
  deallocate (in_data_r8)

  call adios2_finalize(adios, ierr)
  call MPI_Finalize(ierr)

end program TestBPWriteMemorySelectionRead2D
