 program TestBPWriteTypes
   use small_test_data
   use mpi
   use adios2
   implicit none


   integer(kind=8) :: adios, ioRead, bpReader
   integer(kind=8), dimension(1) :: shape_dims, start_dims, count_dims
   integer :: inx, irank, isize, ierr, i, n

   type(adios2_file) :: adios2_fhw, adios2_fhr
   integer(kind=8), dimension(12) :: variables
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
   real,            dimension(:), allocatable :: in_R32
   real(kind=8),    dimension(:), allocatable :: in_R64

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
   call adios2_fopen(adios2_fhw, 'ftypes_hl.bp', adios2_mode_write, &
                     MPI_COMM_WORLD, ierr)

   ! Global single value variables
   if (irank == 0) then
     call adios2_fwrite(adios2_fhw, "gvar_I8", data_I8(1), ierr)
     call adios2_fwrite(adios2_fhw, "gvar_I16", data_I16(1), ierr)
     call adios2_fwrite(adios2_fhw, "gvar_I32", data_I32(1), ierr)
     call adios2_fwrite(adios2_fhw, "gvar_I64", data_I64(1), ierr)
     call adios2_fwrite(adios2_fhw, "gvar_R32", data_R32(1), ierr)
     call adios2_fwrite(adios2_fhw, "gvar_R64", data_R64(1), ierr)
   end if

   ! 1D array variables
   do i = 1, 3
     call adios2_fwrite(adios2_fhw, "var_I8", data_I8, 1, shape_dims, &
                            start_dims, count_dims, ierr)

     call adios2_fwrite(adios2_fhw, "var_I16", data_I16, 1, shape_dims, &
                            start_dims, count_dims, ierr)

     call adios2_fwrite(adios2_fhw, "var_I32", data_I32, 1, shape_dims, &
                            start_dims, count_dims, ierr)

     call adios2_fwrite(adios2_fhw, "var_I64", data_I64, 1, shape_dims, &
                            start_dims, count_dims, ierr)

     call adios2_fwrite(adios2_fhw, "var_R32", data_R32, 1, shape_dims, &
                            start_dims, count_dims, ierr)

     call adios2_fwrite(adios2_fhw, "var_R64", data_R64, 1, shape_dims, &
                            start_dims, count_dims, ADIOS2_ADVANCE_YES, ierr)
   end do

   call adios2_fclose(adios2_fhw, ierr)

   !!!!!!!!!!!!!!!!!!!!!!!!! High-level READER !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   call adios2_fopen(adios2_fhr, 'ftypes_hl.bp', adios2_mode_read, &
                     MPI_COMM_WORLD, ierr)

   call adios2_fread(adios2_fhr, "gvar_I8", in_gI8, ierr)
   write(*,*) 'in_gI8 ', in_gI8
   if(in_gI8 /= data_I8(1) ) stop  "in_I8 not equal data_I8(1)"

   call adios2_fread(adios2_fhr, "gvar_I16", in_gI16, ierr)
   write(*,*) 'in_gI16 ', in_gI16
   if(in_gI16 /= data_I16(1) ) stop  "in_gI16 not equal data_I16(1)"

   call adios2_fread(adios2_fhr, "gvar_I32", in_gI32, ierr)
   if(in_gI32 /= data_I32(1) ) stop  "in_gI32 not equal data_I32(1)"

   call adios2_fread(adios2_fhr, "gvar_I64", in_gI64, ierr)
   if(in_gI64 /= data_I64(1) ) stop  "in_gI64 not equal data_I64(1)"

   call adios2_fread(adios2_fhr, "gvar_R32", in_gR32, ierr)
   if(in_gR32 /= data_R32(1) ) stop  "in_gR32 not equal data_R32(1)"

   call adios2_fread(adios2_fhr, "gvar_R64", in_gR64, ierr)
   if(in_gR64 /= data_R64(1) ) stop  "in_gR64 not equal data_R64(1)"


   do i = 1, 3
     call adios2_fread(adios2_fhr, "var_I8", in_I8, start_dims, count_dims, ierr)
     call adios2_fread(adios2_fhr, "var_I16", in_I16, start_dims, count_dims, ierr)
     call adios2_fread(adios2_fhr, "var_I32", in_I32, start_dims, count_dims, ierr)
     call adios2_fread(adios2_fhr, "var_I64", in_I64, start_dims, count_dims, ierr)
     call adios2_fread(adios2_fhr, "var_R32", in_R32, start_dims, count_dims, ierr)
     call adios2_fread(adios2_fhr, "var_R64", in_R64, start_dims, count_dims, &
                       adios2_advance_yes, ierr)

     do n = 1, count_dims(1)
         if(in_I8(n) /= data_I8(n) ) stop 'I8 read failure'
         if(in_I16(n) /= data_I16(n) ) stop 'I16 read failure'
         if(in_I32(n) /= data_I32(n) ) stop 'I32 read failure'
         if(in_I64(n) /= data_I64(n) ) stop 'I64 read failure'
         if(in_R32(n) /= data_R32(n) ) stop 'R32 read failure'
         if(in_R64(n) /= data_R64(n) ) stop 'R64 read failure'
     end do
   end do

   if( allocated(in_I8) ) deallocate(in_I8)
   if( allocated(in_I16) ) deallocate(in_I16)
   if( allocated(in_I32) ) deallocate(in_I32)
   if( allocated(in_I64) ) deallocate(in_I64)
   if( allocated(in_R32) ) deallocate(in_R32)
   if( allocated(in_R64) ) deallocate(in_R64)

   call adios2_fclose(adios2_fhr, ierr)

   !!!!!!!!!!!!!!!!!!!!!!!!! Low-level READER !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   call adios2_init(adios, MPI_COMM_WORLD, adios2_debug_mode_on, ierr)
   ! Declare io reader
   call adios2_declare_io(ioRead, adios, "ioRead", ierr)
   ! Open bpReader engine
   call adios2_open(bpReader, ioRead, "ftypes_hl.bp", adios2_mode_read, ierr)

   call adios2_inquire_variable(variables(1), ioRead, "var_I8", ierr)
   call adios2_variable_name(variables(1), variable_name, ierr)
   if (variable_name /= 'var_I8') stop  'var_I8 not recognized'
   call adios2_variable_type(variables(1), variable_type, ierr)
   if (variable_type /= adios2_type_integer1) stop  'var_I8 type not recognized'
   call adios2_variable_shape(variables(1), ndims, shape_in, ierr)
   if (ndims /= 1) stop  'var_I8 ndims is not 1'
   if (shape_in(1) /= isize*inx) stop  'var_I8 shape_in read failed'

   call adios2_inquire_variable(variables(2), ioRead, "var_I16", ierr)
   call adios2_variable_name(variables(2), variable_name, ierr)
   if (variable_name /= 'var_I16') stop 'var_I16 not recognized'
   call adios2_variable_type(variables(2), variable_type, ierr)
   if (variable_type /= adios2_type_integer2) stop 'var_I16 type not recognized'
   call adios2_variable_shape(variables(2), ndims, shape_in, ierr)
   if (ndims /= 1) stop 'var_I16 ndims is not 1'
   if (shape_in(1) /= isize*inx) stop 'var_I16 shape_in read failed'

   call adios2_inquire_variable(variables(3), ioRead, "var_I32", ierr)
   call adios2_variable_name(variables(3), variable_name, ierr)
   if (variable_name /= 'var_I32') stop 'var_I32 not recognized'
   call adios2_variable_type(variables(3), variable_type, ierr)
   if (variable_type /= adios2_type_integer4) stop 'var_I32 type not recognized'
   call adios2_variable_shape(variables(3), ndims, shape_in, ierr)
   if (ndims /= 1) stop 'var_I32 ndims is not 1'
   if (shape_in(1) /= isize*inx) stop 'var_I32 shape_in read failed'

   call adios2_inquire_variable(variables(4), ioRead, "var_I64", ierr)
   call adios2_variable_name(variables(4), variable_name, ierr)
   if (variable_name /= 'var_I64') stop 'var_I64 not recognized'
   call adios2_variable_type(variables(4), variable_type, ierr)
   if (variable_type /= adios2_type_integer8) stop 'var_I64 type not recognized'
   call adios2_variable_shape(variables(4), ndims, shape_in, ierr)
   if (ndims /= 1) stop 'var_I64 ndims is not 1'
   if (shape_in(1) /= isize*inx) stop 'var_I64 shape_in read failed'

   call adios2_inquire_variable(variables(5), ioRead, "var_R32", ierr)
   call adios2_variable_name(variables(5), variable_name, ierr)
   if (variable_name /= 'var_R32') stop 'var_R32 not recognized'
   call adios2_variable_type(variables(5), variable_type, ierr)
   if (variable_type /= adios2_type_real) stop 'var_R32 type not recognized'
   call adios2_variable_shape(variables(5), ndims, shape_in, ierr)
   if (ndims /= 1) stop 'var_R32 ndims is not 1'
   if (shape_in(1) /= isize*inx) stop 'var_R32 shape_in read failed'

   call adios2_inquire_variable(variables(6), ioRead, "var_R64", ierr)
   call adios2_variable_name(variables(6), variable_name, ierr)
   if (variable_name /= 'var_R64') stop 'var_R64 not recognized'
   call adios2_variable_type(variables(6), variable_type, ierr)
   if (variable_type /= adios2_type_dp) stop 'var_R64 type not recognized'
   call adios2_variable_shape(variables(6), ndims, shape_in, ierr)
   if (ndims /= 1) stop 'var_R64 ndims is not 1'
   if (shape_in(1) /= isize*inx) stop 'var_R64 shape_in read failed'

   call adios2_close(bpReader, ierr)

   ! Deallocates adios and calls its destructor
   call adios2_finalize(adios, ierr)

   call MPI_Finalize(ierr)

 end program TestBPWriteTypes
