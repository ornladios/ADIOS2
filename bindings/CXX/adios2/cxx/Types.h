/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_BINDINGS_CXX_TYPES_H_
#define ADIOS2_BINDINGS_CXX_TYPES_H_

#include "Variable.h"

#include <vector>

namespace adios2
{

template <class T>
std::string GetType() noexcept;

} // end namespace adios2
#endif /* ADIOS2_BINDINGS_CXX_TYPES_H_ */
