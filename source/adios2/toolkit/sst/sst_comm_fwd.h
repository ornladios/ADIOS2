/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_TOOLKIT_SST_SST_COMM_FWD_H_
#define ADIOS2_TOOLKIT_SST_SST_COMM_FWD_H_

#ifdef __cplusplus
namespace adios2
{
namespace helper
{
class Comm;
}
}

// In C++ we use the real Comm type.
typedef adios2::helper::Comm *SMPI_Comm;
#else
// In C we cannot access the real Comm type, so use an opaque pointer.
// We never dereference it in C, but we can carry it around.
typedef struct SMPI_Comm_s *SMPI_Comm;
#endif

#endif /* ADIOS2_TOOLKIT_SST_SST_COMM_FWD_H_ */
