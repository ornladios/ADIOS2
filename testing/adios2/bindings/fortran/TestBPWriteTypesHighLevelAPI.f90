 program TestBPWriteTypesHighLevelAPI
   use small_test_data
   use mpi
   use adios2
   implicit none

   ! high-level writer
   type(adios2_file) :: fhw, fhr
   ! low-level reader
   type(adios2_adios) :: adios
   type(adios2_io) :: ioRead
   type(adios2_variable), dimension(12) :: variables
   type(adios2_engine) :: bpReader

   integer(kind=8), dimension(1) :: shape_dims, start_dims, count_dims
   integer :: inx, irank, isize, ierr, i, n

   ! read handlers
   character(len=:), allocatable :: variable_name
   integer :: variable_type, ndims
   integer(kind=8), dimension(:), allocatable :: shape_in

   ! read arrays
   integer(kind=1) :: in_gi8
   integer(kind=2) :: in_gi16
   integer(kind=4) :: in_gi32
   integer(kind=8) :: in_gi64
   real(kind=4)    :: in_gr32
   real(kind=8)    :: in_gr64

   integer(kind=1), dimension(:), allocatable :: in_I8
   integer(kind=2), dimension(:), allocatable :: in_I16
   integer(kind=4), dimension(:), allocatable :: in_I32
   integer(kind=8), dimension(:), allocatable :: in_I64
   real, dimension(:), allocatable :: in_R32
   real(kind=8), dimension(:), allocatable :: in_R64

   ! Launch MPI
   call MPI_Init(ierr)
   call MPI_Comm_rank(MPI_COMM_WORLD, irank, ierr)
   call MPI_Comm_size(MPI_COMM_WORLD, isize, ierr)

   ! Application variables
   inx = 10

   ! Variable dimensions
   shape_dims(1) = isize*inx
   start_dims(1) = irank*inx
   count_dims(1) = inx

   !!!!!!!!!!!!!!!!!!!!!!!!! WRITER !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   call adios2_fopen(fhw, 'ftypes_hl.bp', adios2_mode_write, &
                     MPI_COMM_WORLD, ierr)

   ! Global single value variables
   if (irank == 0) then
     call adios2_fwrite(fhw, "gvar_I8", data_I8(1), ierr)
     call adios2_fwrite(fhw, "gvar_I16", data_I16(1), ierr)
     call adios2_fwrite(fhw, "gvar_I32", data_I32(1), ierr)
     call adios2_fwrite(fhw, "gvar_I64", data_I64(1), ierr)
     call adios2_fwrite(fhw, "gvar_R32", data_R32(1), ierr)
     call adios2_fwrite(fhw, "gvar_R64", data_R64(1), ierr)
   end if

   ! 1D array variables
   do i = 1, 3
     call adios2_fwrite(fhw, "var_I8", data_I8, 1, shape_dims, &
                        start_dims, count_dims, ierr)

     call adios2_fwrite(fhw, "var_I16", data_I16, 1, shape_dims, &
                        start_dims, count_dims, ierr)

     call adios2_fwrite(fhw, "var_I32", data_I32, 1, shape_dims, &
                        start_dims, count_dims, ierr)

     call adios2_fwrite(fhw, "var_I64", data_I64, 1, shape_dims, &
                        start_dims, count_dims, ierr)

     call adios2_fwrite(fhw, "var_R32", data_R32, 1, shape_dims, &
                        start_dims, count_dims, ierr)

     call adios2_fwrite(fhw, "var_R64", data_R64, 1, shape_dims, &
                        start_dims, count_dims, ADIOS2_ADVANCE_YES, ierr)
   end do

   call adios2_fclose(fhw, ierr)

   !!!!!!!!!!!!!!!!!!!!!!!!! High-level READER !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   call adios2_fopen(fhr, 'ftypes_hl.bp', adios2_mode_read, &
                     MPI_COMM_WORLD, ierr)

   i = 0
   do

     if (i == 0) then
       call adios2_fread(fhr, "gvar_I8", in_gI8, ierr)
       write (*, *) 'in_gI8 ', in_gI8
       if (in_gI8 /= data_I8(1)) stop "in_I8 not equal data_I8(1)"

       call adios2_fread(fhr, "gvar_I16", in_gI16, ierr)
       write (*, *) 'in_gI16 ', in_gI16
       if (in_gI16 /= data_I16(1)) stop "in_gI16 not equal data_I16(1)"

       call adios2_fread(fhr, "gvar_I32", in_gI32, ierr)
       if (in_gI32 /= data_I32(1)) stop "in_gI32 not equal data_I32(1)"

       call adios2_fread(fhr, "gvar_I64", in_gI64, ierr)
       if (in_gI64 /= data_I64(1)) stop "in_gI64 not equal data_I64(1)"

       call adios2_fread(fhr, "gvar_R32", in_gR32, ierr)
       if (in_gR32 /= data_R32(1)) stop "in_gR32 not equal data_R32(1)"

       call adios2_fread(fhr, "gvar_R64", in_gR64, ierr)
       if (in_gR64 /= data_R64(1)) stop "in_gR64 not equal data_R64(1)"
     end if

     call adios2_fread(fhr, "var_I8", in_I8, start_dims, count_dims, ierr)
     call adios2_fread(fhr, "var_I16", in_I16, start_dims, count_dims, ierr)
     call adios2_fread(fhr, "var_I32", in_I32, start_dims, count_dims, ierr)
     call adios2_fread(fhr, "var_I64", in_I64, start_dims, count_dims, ierr)
     call adios2_fread(fhr, "var_R32", in_R32, start_dims, count_dims, ierr)
     call adios2_fread(fhr, "var_R64", in_R64, start_dims, count_dims, &
                       ADIOS2_ADVANCE_YES, ierr)

     do n = 1, count_dims(1)
       if (in_I8(n) /= data_I8(n)) stop 'I8 read failure'
       if (in_I16(n) /= data_I16(n)) stop 'I16 read failure'
       if (in_I32(n) /= data_I32(n)) stop 'I32 read failure'
       if (in_I64(n) /= data_I64(n)) stop 'I64 read failure'
       if (in_R32(n) /= data_R32(n)) stop 'R32 read failure'
       if (in_R64(n) /= data_R64(n)) stop 'R64 read failure'
     end do

     i = i + 1
     if (ierr < 0) exit

   end do

   call adios2_fclose(fhr, ierr)

   if (allocated(in_I8)) deallocate (in_I8)
   if (allocated(in_I16)) deallocate (in_I16)
   if (allocated(in_I32)) deallocate (in_I32)
   if (allocated(in_I64)) deallocate (in_I64)
   if (allocated(in_R32)) deallocate (in_R32)
   if (allocated(in_R64)) deallocate (in_R64)

   call MPI_Finalize(ierr)

 end program TestBPWriteTypesHighLevelAPI
