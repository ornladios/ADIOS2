/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Variable.tcc : implementation of private template functions
 *
 *  Created on: Feb 12, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_CXX11_CXX11_VARIABLE_TCC_
#define ADIOS2_BINDINGS_CXX11_CXX11_VARIABLE_TCC_

#include "Variable.h"

#include "adios2/helper/adiosFunctions.h"

namespace adios2
{

namespace
{

template <class T>
std::vector<typename Variable<T>::Info>
ToBlocksInfo(const std::vector<typename core::Variable<
                 typename TypeInfo<T>::IOType>::Info> &coreBlocksInfo)
{
    using IOType = typename TypeInfo<T>::IOType;

    std::vector<typename Variable<T>::Info> blocksInfo;
    blocksInfo.reserve(coreBlocksInfo.size());

    for (const typename core::Variable<IOType>::Info &coreBlockInfo :
         coreBlocksInfo)
    {
        typename Variable<T>::Info blockInfo;
        blockInfo.Start = coreBlockInfo.Start;
        blockInfo.Count = coreBlockInfo.Count;
        blockInfo.BlockID = coreBlockInfo.BlockID;
        blockInfo.Step = coreBlockInfo.Step;
        blockInfo.WriterID = coreBlockInfo.WriterID;

        blockInfo.IsReverseDims = coreBlockInfo.IsReverseDims;
        blockInfo.IsValue = coreBlockInfo.IsValue;

        if (blockInfo.IsValue)
        {
            blockInfo.Value = coreBlockInfo.Value;
        }
        else
        {
            blockInfo.Min = coreBlockInfo.Min;
            blockInfo.Max = coreBlockInfo.Max;
        }

        blocksInfo.push_back(blockInfo);
    }

    return blocksInfo;
}
} // end empty namespace

template <class T>
std::vector<std::vector<typename Variable<T>::Info>>
Variable<T>::DoAllStepsBlocksInfo()
{
    helper::CheckForNullptr(m_Variable,
                            "in call to Variable<T>::AllStepsBlocksInfo");

    // PRIVATE INPUT
    const std::vector<std::vector<typename core::Variable<IOType>::Info>>
        coreAllStepsBlocksInfo = m_Variable->AllStepsBlocksInfo();

    // PUBLIC OUTPUT
    std::vector<std::vector<typename Variable<T>::Info>> allStepsBlocksInfo(
        coreAllStepsBlocksInfo.size());

    size_t relativeStep = 0;
    for (const auto &coreBlocksInfo : coreAllStepsBlocksInfo)
    {
        allStepsBlocksInfo[relativeStep] = ToBlocksInfo<T>(coreBlocksInfo);
        ++relativeStep;
    }
    return allStepsBlocksInfo;
}

template <typename T>
std::string ToString(const Variable<T> &variable)
{
    return std::string("Variable<") + variable.Type() + ">(Name: \"" +
           variable.Name() + "\")";
}

namespace detail
{
// Span
template <class T>
Span<T>::Span(core::Span<IOType> *coreSpan) : m_Span(coreSpan)
{
}

template <class T>
size_t Span<T>::size() const noexcept
{
    return m_Span->Size();
}

template <class T>
T *Span<T>::data() const noexcept
{
    return reinterpret_cast<T *>(m_Span->Data());
}

template <class T>
T &Span<T>::at(const size_t position)
{
    IOType &data = m_Span->At(position);
    return reinterpret_cast<T &>(data);
}

template <class T>
const T &Span<T>::at(const size_t position) const
{
    const IOType &data = m_Span->At(position);
    return reinterpret_cast<const T &>(data);
}

template <class T>
T &Span<T>::operator[](const size_t position)
{
    IOType &data = (*m_Span)[position];
    return reinterpret_cast<T &>(data);
}

template <class T>
const T &Span<T>::operator[](const size_t position) const
{
    const IOType &data = (*m_Span)[position];
    return reinterpret_cast<const T &>(data);
}

} // end namespace detail

} // end namespace adios2

#endif /* ADIOS2_BINDINGS_CXX11_CXX11_VARIABLE_TCC_ */
