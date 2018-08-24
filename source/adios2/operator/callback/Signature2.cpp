/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Signature2.cpp
 *
 *  Created on: Oct 20, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "Signature2.h"

namespace adios2
{
namespace core
{
namespace callback
{

Signature2::Signature2(
    const std::function<void(void *, const std::string &, const std::string &,
                             const std::string &, const size_t, const Dims &,
                             const Dims &, const Dims &)> &function,
    const Params &parameters, const bool debugMode)
: Operator("Signature2", parameters, debugMode), m_Function(function)
{
}

void Signature2::RunCallback2(void *arg1, const std::string &arg2,
                              const std::string &arg3, const std::string &arg4,
                              const size_t arg5, const Dims &arg6,
                              const Dims &arg7, const Dims &arg8) const
{
    if (m_Function)
    {
        m_Function(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
    }
    else
    {
        throw std::runtime_error(
            "ERROR: callback function of Signature2 type failed\n");
    }
}

} // end namespace callback
} // end namespace core
} // end namespace adios2
