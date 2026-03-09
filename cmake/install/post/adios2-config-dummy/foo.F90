! SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
!
! SPDX-License-Identifier: Apache-2.0

subroutine foo
#ifdef WITH_ADIOS2
  use adios2
#endif
end subroutine foo
