/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_BINDINGS_CXX_TYPES_TCC_
#define ADIOS2_BINDINGS_CXX_TYPES_TCC_

#include "Types.h"

#include "adios2/helper/adiosFunctions.h"

namespace adios2
{

template <class T>
std::string GetType() noexcept
{
    return ToString(helper::GetDataType<typename TypeInfo<T>::IOType>());
}

} // end namespace adios2

#endif /* ADIOS2_BINDINGS_CXX_TYPES_TCC_ */
