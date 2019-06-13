 program TestNullEngine
     use mpi
     use adios2
     implicit none

     integer(kind=8), dimension(1) :: count_dims
     integer :: inx, irank, isize, ierr, step_status, s

     type(adios2_adios) :: adios
     type(adios2_io) :: ioWrite, ioRead
     type(adios2_variable) :: var, varIn
     type(adios2_engine) :: nullWriter, nullReader

     ! data
     real(kind=8), dimension(10) :: R64, inR64

     ! Program starts

     ! Launch MPI
     call MPI_Init(ierr)
     call MPI_Comm_rank(MPI_COMM_WORLD, irank, ierr)
     call MPI_Comm_size(MPI_COMM_WORLD, isize, ierr)

     ! Application variables
     inx = 10
     R64 = 0.
     inR64 = 0.

     ! Variable dimensions
     count_dims(1) = inx

     ! Create adios handler passing the communicator, debug mode and error flag
     call adios2_init(adios, MPI_COMM_WORLD, adios2_debug_mode_on, ierr)

     !!!!!!!!!!!!!!!!!!!!!!!!! WRITER !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
     ! Declare an IO process configuration inside adios
     call adios2_declare_io(ioWrite, adios, "nullWriter", ierr)
     call adios2_set_engine(ioWrite, "NULL", ierr)
     if (TRIM(ioWrite%engine_type) /= "NULL") stop 'Wrong io engine_type'

     ! Defines a variable to be written in bp format
     call adios2_define_variable(var, ioWrite, "var_R64", &
                                 adios2_type_dp, 1, &
                                 adios2_null_dims, adios2_null_dims, count_dims, &
                                 adios2_constant_dims, ierr)

     call adios2_open(nullWriter, ioWrite, "fnull.bp", adios2_mode_write, ierr)

     do s = 1, 3
         call adios2_begin_step(nullWriter, ierr)
         call adios2_put(nullWriter, var, R64, ierr)
         call adios2_perform_puts(nullWriter, ierr)
         call adios2_end_step(nullWriter, ierr)
     end do

     call adios2_close(nullWriter, ierr)

     !!!!!!!!!!!!!!!!!!!!!!!!! READER !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
     ! Declare io reader
     call adios2_declare_io(ioRead, adios, "nullReader", ierr)
     call adios2_set_engine(ioRead, "null", ierr)

     ! Open nullReader engine
     call adios2_open(nullReader, ioRead, "fnull.bp", adios2_mode_read, ierr)

     do s = 1, 3
         call adios2_begin_step(nullReader, adios2_step_mode_read, -1.0, &
                                step_status, ierr)
         if (step_status /= adios2_step_status_end_of_stream) then
             stop 'null engine status failed'
         end if

         call adios2_inquire_variable(varIn, ioRead, "var_R64", ierr)
         if (varIn%valid .eqv. .true.) stop 'var_R64 inquire error'

         call adios2_get(nullReader, varIn, inR64, ierr)
         call adios2_perform_gets(nullReader, ierr)
         call adios2_end_step(nullReader, ierr)
     end do

     call adios2_close(nullReader, ierr)
     call adios2_finalize(adios, ierr)

     call MPI_Finalize(ierr)

 end program TestNullEngine
