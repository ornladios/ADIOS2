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

Operator::Operator(const std::string type, const Params &parameters)
: m_Type(type), m_Parameters(parameters)
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
    throw std::invalid_argument("ERROR: signature (const size_t) not supported "
                                "by derived class implemented with " +
                                m_Type + ", in call to BufferMaxSize\n");
}

#define declare_type(T)                                                        \
    template <>                                                                \
    size_t Operator::BufferMaxSize<T>(const T *dataIn, const Dims &dimensions, \
                                      const Params &parameters) const          \
    {                                                                          \
        return DoBufferMaxSize(dataIn, dimensions, helper::GetDataType<T>(),   \
                               parameters);                                    \
    }
ADIOS2_FOREACH_ZFP_TYPE_1ARG(declare_type)
#undef declare_type

size_t Operator::Compress(const void * /*dataIn*/, const Dims & /*dimensions*/,
                          const size_t /*elementSize*/, DataType /*type*/,
                          void * /*bufferOut*/, const Params & /*params*/,
                          Params & /*info*/) const
{
    throw std::invalid_argument("ERROR: signature (const void*, const "
                                "Dims, const size_t, const std::string, "
                                "void*, const Params&) not supported "
                                "by derived class implemented with " +
                                m_Type + ", in call to Compress\n");
}

size_t Operator::Decompress(const void *bufferIn, const size_t sizeIn,
                            void *dataOut, const size_t sizeOut,
                            Params &info) const
{
    throw std::invalid_argument(
        "ERROR: signature (const void*, const size_t, void) not supported "
        "by derived class implemented with " +
        m_Type + ", in call to Decompress\n");
}

size_t Operator::Decompress(const void * /*bufferIn*/, const size_t /*sizeIn*/,
                            void * /*dataOut*/, const Dims & /*dimensions*/,
                            DataType /*type*/,
                            const Params & /*parameters*/) const
{
    throw std::invalid_argument("ERROR: signature (const void*, const "
                                "size_t, void*, const Dims&, const "
                                "std::string ) not supported "
                                "by derived class implemented with " +
                                m_Type + ", in call to Decompress\n");
}

// PROTECTED
size_t Operator::DoBufferMaxSize(const void *dataIn, const Dims &dimensions,
                                 DataType type, const Params &parameters) const
{
    throw std::invalid_argument("ERROR: signature (const void*, const Dims& "
                                "std::string ) not supported "
                                "by derived class implemented with " +
                                m_Type + ", in call to BufferMaxSize\n");
}

Dims Operator::ConvertDims(const Dims &dimensions, const DataType type,
                           const size_t targetDims,
                           const bool enforceDims) const
{

    if (targetDims < 1)
    {
        throw(std::invalid_argument(
            "Operator::ConvertDims only accepts targetDims>0"));
    }

    Dims ret = dimensions;

    while (true)
    {
        auto it = std::find(ret.begin(), ret.end(), 1);
        if (it == ret.end())
        {
            break;
        }
        else
        {
            ret.erase(it);
        }
    }

    while (ret.size() > targetDims)
    {
        ret[1] *= ret[0];
        ret.erase(ret.begin());
    }

    if (enforceDims && ret.size() < targetDims)
    {
        ret.insert(ret.begin(), 1);
    }

    if (type == helper::GetDataType<std::complex<float>>() ||
        type == helper::GetDataType<std::complex<double>>())
    {
        ret.back() *= 2;
    }
    return ret;
}

// PRIVATE
void Operator::CheckCallbackType(const std::string type) const
{
    if (m_Type != type)
    {
        throw std::invalid_argument("ERROR: operator of type " + m_Type +
                                    " doesn't match expected callback type " +
                                    type + " arguments\n");
    }
}

} // end namespace core
} // end namespace adios2
