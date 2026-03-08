/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_HELPER_ADIOSCOMMDUMMY_H_
#define ADIOS2_HELPER_ADIOSCOMMDUMMY_H_

#include "adiosComm.h"

namespace adios2
{
namespace helper
{

/**
 * @brief Create a dummy communicator.
 */
Comm CommDummy();

} // end namespace helper
} // end namespace adios2

#endif // ADIOS2_HELPER_ADIOSCOMMDUMMY_H_
