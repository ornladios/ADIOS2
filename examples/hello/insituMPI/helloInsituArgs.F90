module helloInsituArgs
    implicit none
    character(len=256), parameter :: streamname = "insitu_stream"
    ! arguments
    character(len=256) :: xmlfile
    integer :: npx, npy    ! # of processors in x-y direction
    integer :: posx, posy  ! position of rank in 2D decomposition
    integer(kind=8) :: ndx, ndy    ! size of array per processor (without ghost cells)
    integer(kind=8) :: offx, offy  ! Offsets of local array in this process
    integer :: steps       ! number of steps to write
    integer :: sleeptime   ! wait time between steps in seconds

contains

!!***************************
subroutine usage(isWriter)
    logical, intent(in) :: isWriter
    if (isWriter) then
        print *, "Usage: helloInsituMPIWriter  config  N  M   nx  ny   steps  sleeptime"
    else
        print *, "Usage: helloInsituMPIReader  config  N  M"
    endif
    print *, " "
    print *, "config:    path of XML config file"
    print *, "N:         number of processes in X dimension"
    print *, "M:         number of processes in Y dimension"
    if (isWriter) then
        print *, "nx:        local array size in X dimension per processor"
        print *, "ny:        local array size in Y dimension per processor"
        print *, "steps:     the total number of steps to output"
        print *, "sleeptime: wait time between steps in seconds"
    endif
end subroutine usage

!!***************************
subroutine processArgs(nproc, isWriter)

#if defined(ADIOS2_HAVE_FORTRAN_F03_ARGS)
# define ADIOS2_ARGC() command_argument_count()
# define ADIOS2_ARGV(i, v) call get_command_argument(i, v)
#elif defined(ADIOS2_HAVE_FORTRAN_GNU_ARGS)
# define ADIOS2_ARGC() iargc()
# define ADIOS2_ARGV(i, v) call getarg(i, v)
#else
# define ADIOS2_ARGC() 1
# define ADIOS2_ARGV(i, v)
#endif

    integer, intent(in) :: nproc
    logical, intent(in) :: isWriter
    character(len=256) :: npx_str, npy_str, ndx_str, ndy_str
    character(len=256) :: steps_str,time_str
    integer :: numargs, expargs

    !! expected number of arguments
    if (isWriter) then
        expargs = 7
    else
        expargs = 3
    endif

    !! process arguments
    numargs = ADIOS2_ARGC()
    !print *,"Number of arguments:",numargs
    if ( numargs < expargs ) then
        call usage(isWriter)
        call exit(1)
    endif
    ADIOS2_ARGV(1, xmlfile)
    ADIOS2_ARGV(2, npx_str)
    ADIOS2_ARGV(3, npy_str)
    if (isWriter) then
        ADIOS2_ARGV(4, ndx_str)
        ADIOS2_ARGV(5, ndy_str)
        ADIOS2_ARGV(6, steps_str)
        ADIOS2_ARGV(7, time_str)
    endif
    read (npx_str,'(i5)') npx
    read (npy_str,'(i5)') npy
    if (isWriter) then
        read (ndx_str,'(i6)') ndx
        read (ndy_str,'(i6)') ndy
        read (steps_str,'(i6)') steps
        read (time_str,'(i6)') sleeptime
    endif

    if (npx*npy /= nproc) then
        print *,"ERROR: N*M must equal the number of processes"
        call usage(isWriter)
        call exit(1)
    endif

end subroutine processArgs

!!******************************************************
subroutine DecomposeArray(gndx, gndy, rank)
    integer(kind=8), intent(in) :: gndx
    integer(kind=8), intent(in) :: gndy
    integer, intent(in) :: rank

    posx = mod(rank, npx);
    posy = rank / npx;

    ! 2D decomposition of global array reading
    ndx = gndx / npx;
    ndy = gndy / npy;
    offx = ndx * posx;
    offy = ndy * posy;
    if (posx == npx - 1) then
        ! right-most processes need to read all the rest of rows
        ndx = gndx - ndx * (npx - 1);
    endif

    if (posy == npy - 1) then
        ! bottom processes need to read all the rest of columns
        ndy = gndy - ndy * (npy - 1);
    endif
end subroutine DecomposeArray


end module helloInsituArgs
