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

#include <iostream>

namespace adios2
{
namespace core
{

Operator::Operator(const std::string &typeString, const OperatorType typeEnum,
                   const std::string &category, const Params &parameters)
: m_TypeString(typeString), m_TypeEnum(typeEnum), m_Category(category),
  m_Parameters(helper::LowerCaseParams(parameters))
{
}

void Operator::SetParameter(const std::string key, const std::string value) noexcept
{
    m_Parameters[helper::LowerCase(key)] = value;
}

Params &Operator::GetParameters() noexcept { return m_Parameters; }

void Operator::SetAccuracy(const adios2::Accuracy &a) noexcept { m_AccuracyRequested = a; }
adios2::Accuracy Operator::GetAccuracy() const noexcept { return m_AccuracyProvided; }

// PROTECTED

Dims Operator::ConvertDims(const Dims &dimensions, const DataType type, const size_t targetDims,
                           const bool enforceDims, const size_t defaultDimSize) const
{

    if (targetDims < 1)
    {
        helper::Throw<std::invalid_argument>("Core", "Operator", "ConvertDims",
                                             "only accepts targetDims > 0");
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

size_t Operator::GetHeaderSize() const { return 0; }

size_t Operator::GetEstimatedSize(const size_t ElemCount, const size_t ElemSize, const size_t ndims,
                                  const size_t *dims) const
{
    return ElemCount * ElemSize + 128;
};

void Operator::AddExtraParameters(const Params &params) {}

size_t Operator::Operate(const char *dataIn, const Dims &blockStart, const Dims &blockCount,
                         const DataType type, char *bufferOut)
{
    return 0;
}

size_t Operator::InverseOperate(const char *bufferIn, const size_t sizeIn, char *dataOut)
{
    return 0;
}

} // end namespace core
} // end namespace adios2
