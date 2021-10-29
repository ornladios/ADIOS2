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

size_t Operator::Operate(const char * /*dataIn*/, const Dims & /*blockStart*/,
                         const Dims & /*blockCount*/, const DataType /*type*/,
                         char * /*bufferOut*/, const Params & /*params*/)
{
    throw std::invalid_argument("ERROR: signature (const void*, const "
                                "Dims, const size_t, const std::string, "
                                "void*, const Params&) not supported "
                                "by derived class implemented with " +
                                m_Type + ", in call to Compress\n");
}

size_t Operator::InverseOperate(const char *bufferIn, const size_t sizeIn,
                                char *dataOut)
{
    throw std::invalid_argument(
        "ERROR: signature (const void*, const size_t, void) not supported "
        "by derived class implemented with " +
        m_Type + ", in call to Decompress\n");
}

// PROTECTED

Dims Operator::ConvertDims(const Dims &dimensions, const DataType type,
                           const size_t targetDims, const bool enforceDims,
                           const size_t defaultDimSize) const
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

    while (enforceDims && ret.size() < targetDims)
    {
        ret.insert(ret.begin(), defaultDimSize);
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
