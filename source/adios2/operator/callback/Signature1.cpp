/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Signature1.cpp
 *
 *  Created on: Oct 19, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */
#include "Signature1.h"

namespace adios2
{
namespace core
{
namespace callback
{

#define declare_type(T, L)                                                     \
    Signature1::Signature1(                                                    \
        const std::function<void(const T *, const std::string &,               \
                                 const std::string &, const std::string &,     \
                                 const size_t, const Dims &, const Dims &,     \
                                 const Dims &)> &function,                     \
        const Params &parameters, const bool debugMode)                        \
    : Operator("Signature1", parameters, debugMode), m_Function##L(function)   \
    {                                                                          \
    }
ADIOS2_FOREACH_STDTYPE_2ARGS(declare_type)
#undef declare_type

#define declare_type(T, L)                                                     \
    void Signature1::RunCallback1(                                             \
        const T *arg1, const std::string &arg2, const std::string &arg3,       \
        const std::string &arg4, const size_t arg5, const Dims &arg6,          \
        const Dims &arg7, const Dims &arg8) const                              \
    {                                                                          \
        if (m_Function##L)                                                     \
        {                                                                      \
            m_Function##L(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);     \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            throw std::runtime_error("ERROR: Signature1 with type " +          \
                                     std::string(#L) +                         \
                                     " callback function failed\n");           \
        }                                                                      \
    }
ADIOS2_FOREACH_STDTYPE_2ARGS(declare_type)
#undef declare_type

} // end namespace callback
} // end namespace core
} // end namespace adios2
