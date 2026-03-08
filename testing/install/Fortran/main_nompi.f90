c SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
c
c SPDX-License-Identifier: Apache-2.0

program adios_fortran_test
  use adios2
  integer :: ierr, irank, isize
  type(adios2_adios) :: adios

  call adios2_init(adios, ierr)
  call adios2_finalize(adios, ierr)
end program adios_fortran_test
