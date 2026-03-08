! SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
!
! SPDX-License-Identifier: Apache-2.0

#if ADIOS2_USE_MPI
#error "ADIOS2_USE_MPI is true for source not using ADIOS2 MPI bindings"
#endif
#include "main_nompi.f90"
