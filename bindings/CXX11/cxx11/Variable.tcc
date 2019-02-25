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

template <class T>
Variable<T>::Variable(core::Variable<IOType> *variable) : m_Variable(variable)
{
}

template <class T>
Variable<T>::operator bool() const noexcept
{
    return (m_Variable == nullptr) ? false : true;
}

template <class T>
void Variable<T>::SetShape(const Dims &shape)
{
    helper::CheckForNullptr(m_Variable, "in call to Variable<T>::SetShape");
    m_Variable->SetShape(shape);
}

template <class T>
void Variable<T>::SetBlockSelection(const size_t blockID)
{
    helper::CheckForNullptr(m_Variable,
                            "in call to Variable<T>::SetBlockSelection");
    m_Variable->SetBlockSelection(blockID);
}

template <class T>
void Variable<T>::SetSelection(const Box<Dims> &selection)
{
    helper::CheckForNullptr(m_Variable, "in call to Variable<T>::SetSelection");
    m_Variable->SetSelection(selection);
}

template <class T>
void Variable<T>::SetMemorySelection(const Box<Dims> &memorySelection)
{
    helper::CheckForNullptr(m_Variable,
                            "in call to Variable<T>::SetMemorySelection");
    m_Variable->SetMemorySelection(memorySelection);
}

template <class T>
void Variable<T>::SetStepSelection(const Box<size_t> &stepSelection)
{
    helper::CheckForNullptr(m_Variable,
                            "in call to Variable<T>::SetStepSelection");
    m_Variable->SetStepSelection(stepSelection);
}

template <class T>
size_t Variable<T>::SelectionSize() const
{
    helper::CheckForNullptr(m_Variable,
                            "in call to Variable<T>::SelectionSize");
    return m_Variable->SelectionSize();
}

template <class T>
std::string Variable<T>::Name() const
{
    helper::CheckForNullptr(m_Variable, "in call to Variable<T>::Name");
    return m_Variable->m_Name;
}

template <class T>
std::string Variable<T>::Type() const
{
    helper::CheckForNullptr(m_Variable, "in call to Variable<T>::Type");
    return m_Variable->m_Type;
}
template <class T>
size_t Variable<T>::Sizeof() const
{
    helper::CheckForNullptr(m_Variable, "in call to Variable<T>::Sizeof");
    return m_Variable->m_ElementSize;
}

template <class T>
adios2::ShapeID Variable<T>::ShapeID() const
{
    helper::CheckForNullptr(m_Variable, "in call to Variable<T>::ShapeID");
    return m_Variable->m_ShapeID;
}

template <class T>
Dims Variable<T>::Shape(const size_t step) const
{
    helper::CheckForNullptr(m_Variable, "in call to Variable<T>::Shape");
    return m_Variable->Shape(step);
}

template <class T>
Dims Variable<T>::Start() const
{
    helper::CheckForNullptr(m_Variable, "in call to Variable<T>::Start");
    return m_Variable->m_Start;
}

template <class T>
Dims Variable<T>::Count() const
{
    helper::CheckForNullptr(m_Variable, "in call to Variable<T>::Count");
    return m_Variable->m_Count;
}

template <class T>
size_t Variable<T>::Steps() const
{
    helper::CheckForNullptr(m_Variable, "in call to Variable<T>::Steps");
    return m_Variable->m_AvailableStepsCount;
}

template <class T>
size_t Variable<T>::StepsStart() const
{
    helper::CheckForNullptr(m_Variable, "in call to Variable<T>::StepsStart");
    return m_Variable->m_AvailableStepsStart;
}

template <class T>
size_t Variable<T>::BlockID() const
{
    helper::CheckForNullptr(m_Variable, "in call to Variable<T>::BlockID");
    return m_Variable->m_BlockID;
}

template <class T>
size_t Variable<T>::AddOperation(const Operator op, const Params &parameters)
{
    helper::CheckForNullptr(m_Variable, "in call to Variable<T>::AddOperator");
    if (!op)
    {
        throw std::invalid_argument("ERROR: invalid operator, in call to "
                                    "Variable<T>::AddOperator");
    }
    return m_Variable->AddOperation(*op.m_Operator, parameters);
}

template <class T>
std::vector<typename Variable<T>::Operation> Variable<T>::Operations() const
{
    helper::CheckForNullptr(m_Variable, "in call to Variable<T>::Operations");
    std::vector<Operation> operations;
    operations.reserve(m_Variable->m_Operations.size());

    for (const auto &op : m_Variable->m_Operations)
    {
        operations.push_back(
            Operation{Operator(op.Op), op.Parameters, op.Info});
    }
    return operations;
}

template <class T>
std::pair<T, T> Variable<T>::MinMax(const size_t step) const
{
    helper::CheckForNullptr(m_Variable, "in call to Variable<T>::MinMax");
    return m_Variable->MinMax(step);
}

template <class T>
T Variable<T>::Min(const size_t step) const
{
    helper::CheckForNullptr(m_Variable, "in call to Variable<T>::Min");
    return m_Variable->Min(step);
}

template <class T>
T Variable<T>::Max(const size_t step) const
{
    helper::CheckForNullptr(m_Variable, "in call to Variable<T>::Max");
    return m_Variable->Max(step);
}

template <class T>
std::vector<std::vector<typename Variable<T>::Info>>
Variable<T>::AllStepsBlocksInfo()
{
    return DoAllStepsBlocksInfo();
}

template <class T>
const T *Variable<T>::Info::Data() const
{
    auto coreInfo =
        reinterpret_cast<const typename core::Variable<T>::Info *>(m_Info);

    return m_Info ? (coreInfo->BufferP ? coreInfo->BufferP
                                       : coreInfo->BufferV.data())
                  : nullptr;
}

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
        blockInfo.BlockID = coreBlockInfo.BlockID;
        blockInfo.Step = coreBlockInfo.Step;
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

} // end namespace adios2

#endif /* ADIOS2_BINDINGS_CXX11_CXX11_VARIABLE_TCC_ */
