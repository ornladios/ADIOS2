/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Operator.cpp
 *
 *  Created on: Dec 5, 2016
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "Operator.h"

#include "adios2/common/ADIOSMacros.h"
#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace core
{

Operator::Operator(const std::string type, const Params &parameters,
                   const bool debugMode)
: m_Type(type), m_Parameters(parameters), m_DebugMode(debugMode)
{
}

void Operator::SetParameter(const std::string key,
                            const std::string value) noexcept
{
    m_Parameters[key] = value;
}

Params &Operator::GetParameters() noexcept { return m_Parameters; }

#define declare_type(T)                                                        \
                                                                               \
    void Operator::RunCallback1(                                               \
        const T *arg0, const std::string &arg1, const std::string &arg2,       \
        const std::string &arg3, const size_t arg4, const Dims &arg5,          \
        const Dims &arg6, const Dims &arg7) const                              \
    {                                                                          \
        CheckCallbackType("Callback1");                                        \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void Operator::RunCallback2(void *arg0, const std::string &arg1,
                            const std::string &arg2, const std::string &arg3,
                            const size_t arg4, const Dims &arg5,
                            const Dims &arg6, const Dims &arg7) const
{
    CheckCallbackType("Callback2");
}

size_t Operator::BufferMaxSize(const size_t sizeIn) const
{
    if (m_DebugMode)
    {
        throw std::invalid_argument(
            "ERROR: signature (const size_t) not supported "
            "by derived class implemented with " +
            m_Type + ", in call to BufferMaxSize\n");
    }
    return 0;
}

#define declare_type(T)                                                        \
    template <>                                                                \
    size_t Operator::BufferMaxSize<T>(const T *dataIn, const Dims &dimensions, \
                                      const Params &parameters) const          \
    {                                                                          \
        return DoBufferMaxSize(dataIn, dimensions, helper::GetType<T>(),       \
                               parameters);                                    \
    }
ADIOS2_FOREACH_ZFP_TYPE_1ARG(declare_type)
#undef declare_type

size_t Operator::Compress(const void * /*dataIn*/, const Dims & /*dimensions*/,
                          const size_t /*elementSize*/,
                          const std::string /*type*/, void * /*bufferOut*/,
                          const Params & /*params*/, Params & /*info*/) const
{
    if (m_DebugMode)
    {
        throw std::invalid_argument("ERROR: signature (const void*, const "
                                    "Dims, const size_t, const std::string, "
                                    "void*, const Params&) not supported "
                                    "by derived class implemented with " +
                                    m_Type + ", in call to Compress\n");
    }
    return 0;
}

size_t Operator::Decompress(const void *bufferIn, const size_t sizeIn,
                            void *dataOut, const size_t sizeOut,
                            Params &info) const
{
    if (m_DebugMode)
    {
        throw std::invalid_argument(
            "ERROR: signature (const void*, const size_t, void) not supported "
            "by derived class implemented with " +
            m_Type + ", in call to Decompress\n");
    }

    return 0;
}

size_t Operator::Decompress(const void * /*bufferIn*/, const size_t /*sizeIn*/,
                            void * /*dataOut*/, const Dims & /*dimensions*/,
                            const std::string /*type*/,
                            const Params & /*parameters*/) const
{
    if (m_DebugMode)
    {
        throw std::invalid_argument("ERROR: signature (const void*, const "
                                    "size_t, void*, const Dims&, const "
                                    "std::string ) not supported "
                                    "by derived class implemented with " +
                                    m_Type + ", in call to Decompress\n");
    }

    return 0;
}

// PROTECTED
size_t Operator::DoBufferMaxSize(const void *dataIn, const Dims &dimensions,
                                 const std::string type,
                                 const Params &parameters) const
{
    if (m_DebugMode)
    {
        throw std::invalid_argument(
            "ERROR: signature (const void*, const Dims& "
            "std::string ) not supported "
            "by derived class implemented with " +
            m_Type + ", in call to BufferMaxSize\n");
    }

    return 0;
}

// PRIVATE
void Operator::CheckCallbackType(const std::string type) const
{
    if (m_DebugMode && m_Type != type)
    {
        throw std::invalid_argument("ERROR: operator of type " + m_Type +
                                    " doesn't match expected callback type " +
                                    type + " arguments\n");
    }
}

} // end namespace core
} // end namespace adios2
