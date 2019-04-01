
! ======================================================================
module testing_adios
! ======================================================================
  use adios2
  use mpi
  implicit none

  type(adios2_adios) :: adios

contains
  ! ------------------------------------------------------------
  subroutine testing_adios_init()
  ! ------------------------------------------------------------

    integer :: ierr
    
    call adios2_init(adios, MPI_COMM_WORLD, adios2_debug_mode_on, ierr)
    
  end subroutine testing_adios_init

  ! ------------------------------------------------------------
  subroutine testing_adios_finalize()
  ! ------------------------------------------------------------

    integer :: ierr
    
    call adios2_finalize(adios, ierr)

  end subroutine testing_adios_finalize

end module testing_adios

! ======================================================================
module testing_adios_io
! ======================================================================
  use testing_adios
  use adios2
  implicit none

  type(adios2_io) :: io

contains
  ! ------------------------------------------------------------
  subroutine testing_adios_io_init()
  ! ------------------------------------------------------------

    integer :: ierr

    call testing_adios_init()
    call adios2_declare_io(io, adios, "TestIo", ierr)

  end subroutine testing_adios_io_init

  ! ------------------------------------------------------------
  subroutine testing_adios_io_finalize()
  ! ------------------------------------------------------------

    integer :: ierr
    logical :: result

    ! FIXME, shouldn't we be able to do this by handle?
    call adios2_remove_io(result, adios, "TestIo", ierr)
    if ((ierr /= 0) .or. (result .neqv. .true.)) stop "FAIL: adios2_remove_io"
    call testing_adios_finalize()

  end subroutine testing_adios_io_finalize

end module testing_adios_io

! ======================================================================

! ------------------------------------------------------------
subroutine testing_adios_io_engine()
! ------------------------------------------------------------
! test engine related functionality that's part of IO

  use testing_adios_io
  implicit none

  integer :: ierr
  type(adios2_engine) :: engine

  character(len=:), allocatable :: engine_type

  call testing_adios_io_init()

  ! Engine related functionality
  call adios2_set_engine(io, "bpfile", ierr)

  call adios2_io_engine_type(engine_type, io, ierr)
  if (engine_type /= "bpfile") stop "FAIL adios2_io_engine_type"
  deallocate(engine_type)

  call adios2_open(engine, io, "ftypes.bp", adios2_mode_write, ierr)

  if (engine%type /= "bpfile") stop "FAIL engine%type"
  ! // FIXME, I'd like to check that the engine type itself is correct, but
  ! // there's no (function-style) API to get it
  ! // FIXME, I'd like to check the engine's name, but there's no API to get it

  call adios2_io_engine_type(engine_type, io, ierr)
  if (engine_type /= "bp") stop "FAIL adios2_io_engine_type"
  deallocate(engine_type)

  call testing_adios_io_finalize()

end subroutine testing_adios_io_engine

! ------------------------------------------------------------
subroutine testing_adios_io_engine_default()
! ------------------------------------------------------------
  use testing_adios_io
  implicit none

  integer :: ierr
  type(adios2_engine) :: engine

  character(len=:), allocatable :: engine_type

  call testing_adios_io_init()

  ! Engine related functionality
  call adios2_set_engine(io, "", ierr)

  call adios2_io_engine_type(engine_type, io, ierr)
  if (engine_type /= "") stop "FAIL adios2_io_engine_type"
  deallocate(engine_type)

  call adios2_open(engine, io, "ftypes.bp", adios2_mode_write, ierr)

  if (engine%type /= "") stop "FAIL engine%type"
  ! // FIXME, I'd like to check that the engine type itself is correct, but
  ! // there's no (function-style) API to get it
  ! // FIXME, I'd like to check the engine's name, but there's no API to get it

  call adios2_io_engine_type(engine_type, io, ierr)
  if (engine_type /= "bp") stop "FAIL adios2_io_engine_type"
  deallocate(engine_type)

  call testing_adios_io_finalize

end subroutine testing_adios_io_engine_default

! ======================================================================
program main
! ======================================================================
  use mpi
  implicit none

  integer :: ierr

  call MPI_Init(ierr)

  call testing_adios_io_engine()
  call testing_adios_io_engine_default()

  call MPI_Finalize(ierr)

end program main
