/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * IO.inl inline template functions implementation of IO class. VariableCompound
 * can take any type so must be inlined as type is not known a priori.
 *
 *  Created on: May 15, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_CORE_IO_INL_
#define ADIOS2_CORE_IO_INL_
#ifndef ADIOS2_CORE_IO_H_
#error "Inline file should only be included from it's header, never on it's own"
#endif

#include "IO.h"

namespace adios2
{

template <class T>
VariableCompound &IO::DefineVariableCompound(const std::string &name,
                                             const Dims &shape, const Dims &start,
                                             const Dims &count,
                                             const bool constantDims)
{
    return DefineVariableCompound(name, sizeof(T), shape, start, count,
                                  constantDims);
}

} // end namespace adios2

#endif /* ADIOS2_CORE_IO_INL_ */
