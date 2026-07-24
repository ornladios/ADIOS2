! SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
!
! SPDX-License-Identifier: Apache-2.0


module adios2_io_define_derived_variable_mod
   use adios2_parameters_mod
   implicit none

   external adios2_define_derived_variable_f2c
   external adios2_define_reader_derived_variable_f2c

contains

   subroutine adios2_define_derived_variable(variable, io, name, expression, adios2_derived_var_type, &
      ierr)
      type(adios2_derived_variable), intent(out) :: variable
      type(adios2_io), intent(in) :: io
      character*(*), intent(in) :: name
      character*(*), intent(in) :: expression
      integer, intent(in):: adios2_derived_var_type
      integer, intent(out) :: ierr

      call adios2_define_derived_variable_f2c(variable%f2c, io%f2c, &
         TRIM(ADJUSTL(name))//char(0), TRIM(ADJUSTL(expression))//char(0), &
         adios2_derived_var_type, ierr)
      if( ierr == 0 ) then
         variable%valid = .true.
         variable%name = name
         variable%type = adios2_derived_var_type
      end if

   end subroutine

   ! Reader-side derived variable: the reader computes the expression over
   ! variables in an opened file. No derived-var type and no returned handle
   ! (resolved lazily; find it afterwards with adios2_inquire_variable).
   subroutine adios2_define_reader_derived_variable(io, name, expression, ierr)
      type(adios2_io), intent(in) :: io
      character*(*), intent(in) :: name
      character*(*), intent(in) :: expression
      integer, intent(out) :: ierr

      call adios2_define_reader_derived_variable_f2c(io%f2c, &
         TRIM(ADJUSTL(name))//char(0), TRIM(ADJUSTL(expression))//char(0), ierr)

   end subroutine

end module
