/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Types.h : provides type utilities for ADIOS2 C++11 bindings
 *
 *  Created on: Feb 11, 2019
 *      Author: Kai Germaschewski <kai.germaschewski@unh.edu>
 */

#ifndef ADIOS2_BINDINGS_CXX11_TYPES_H_
#define ADIOS2_BINDINGS_CXX11_TYPES_H_

#include "adios2/helper/adiosType.h"

namespace adios2
{

template <class T>
inline std::string GetType() noexcept
{
    using IOType = typename TypeInfo<T>::IOType;
    return helper::GetType<IOType>();
}

} // end namespace adios2

#endif /* ADIOS2_BINDINGS_CXX11_TYPES_H_ */
