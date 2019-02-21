/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Variable.cpp :
 *
 *  Created on: Jun 4, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */
#include "Variable.h"
#include "Variable.tcc"

#include "adios2/ADIOSMacros.h"
#include "adios2/core/Variable.h"
#include "adios2/helper/adiosFunctions.h" //CheckNullptr

namespace adios2
{

#define declare_type(T)                                                        \
                                                                               \
    template <>                                                                \
    Variable<T>::Variable(core::Variable<IOType> *variable)                    \
    : m_Variable(variable)                                                     \
    {                                                                          \
    }                                                                          \
                                                                               \
    template <>                                                                \
    Variable<T>::operator bool() const noexcept                                \
    {                                                                          \
        return (m_Variable == nullptr) ? false : true;                         \
    }                                                                          \
                                                                               \
    template <>                                                                \
    void Variable<T>::SetShape(const Dims &shape)                              \
    {                                                                          \
        helper::CheckForNullptr(m_Variable,                                    \
                                "in call to Variable<T>::SetShape");           \
        m_Variable->SetShape(shape);                                           \
    }                                                                          \
                                                                               \
    template <>                                                                \
    void Variable<T>::SetBlockSelection(const size_t blockID)                  \
    {                                                                          \
        helper::CheckForNullptr(m_Variable,                                    \
                                "in call to Variable<T>::SetBlockSelection");  \
        m_Variable->SetBlockSelection(blockID);                                \
    }                                                                          \
                                                                               \
    template <>                                                                \
    void Variable<T>::SetSelection(const Box<Dims> &selection)                 \
    {                                                                          \
        helper::CheckForNullptr(m_Variable,                                    \
                                "in call to Variable<T>::SetSelection");       \
        m_Variable->SetSelection(selection);                                   \
    }                                                                          \
                                                                               \
    template <>                                                                \
    void Variable<T>::SetMemorySelection(const Box<Dims> &memorySelection)     \
    {                                                                          \
        helper::CheckForNullptr(m_Variable,                                    \
                                "in call to Variable<T>::SetMemorySelection"); \
        m_Variable->SetMemorySelection(memorySelection);                       \
    }                                                                          \
                                                                               \
    template <>                                                                \
    void Variable<T>::SetStepSelection(const Box<size_t> &stepSelection)       \
    {                                                                          \
        helper::CheckForNullptr(m_Variable,                                    \
                                "in call to Variable<T>::SetStepSelection");   \
        m_Variable->SetStepSelection(stepSelection);                           \
    }                                                                          \
                                                                               \
    template <>                                                                \
    size_t Variable<T>::SelectionSize() const                                  \
    {                                                                          \
        helper::CheckForNullptr(m_Variable,                                    \
                                "in call to Variable<T>::SelectionSize");      \
        return m_Variable->SelectionSize();                                    \
    }                                                                          \
                                                                               \
    template <>                                                                \
    std::string Variable<T>::Name() const                                      \
    {                                                                          \
        helper::CheckForNullptr(m_Variable, "in call to Variable<T>::Name");   \
        return m_Variable->m_Name;                                             \
    }                                                                          \
                                                                               \
    template <>                                                                \
    std::string Variable<T>::Type() const                                      \
    {                                                                          \
        helper::CheckForNullptr(m_Variable, "in call to Variable<T>::Type");   \
        return m_Variable->m_Type.ToString();                                  \
    }                                                                          \
    template <>                                                                \
    size_t Variable<T>::Sizeof() const                                         \
    {                                                                          \
        helper::CheckForNullptr(m_Variable, "in call to Variable<T>::Sizeof"); \
        return m_Variable->m_ElementSize;                                      \
    }                                                                          \
                                                                               \
    template <>                                                                \
    adios2::ShapeID Variable<T>::ShapeID() const                               \
    {                                                                          \
        helper::CheckForNullptr(m_Variable,                                    \
                                "in call to Variable<T>::ShapeID");            \
        return m_Variable->m_ShapeID;                                          \
    }                                                                          \
                                                                               \
    template <>                                                                \
    Dims Variable<T>::Shape(const size_t step) const                           \
    {                                                                          \
        helper::CheckForNullptr(m_Variable, "in call to Variable<T>::Shape");  \
        return m_Variable->Shape(step);                                        \
    }                                                                          \
                                                                               \
    template <>                                                                \
    Dims Variable<T>::Start() const                                            \
    {                                                                          \
        helper::CheckForNullptr(m_Variable, "in call to Variable<T>::Start");  \
        return m_Variable->m_Start;                                            \
    }                                                                          \
                                                                               \
    template <>                                                                \
    Dims Variable<T>::Count() const                                            \
    {                                                                          \
        helper::CheckForNullptr(m_Variable, "in call to Variable<T>::Count");  \
        return m_Variable->m_Count;                                            \
    }                                                                          \
                                                                               \
    template <>                                                                \
    size_t Variable<T>::Steps() const                                          \
    {                                                                          \
        helper::CheckForNullptr(m_Variable, "in call to Variable<T>::Steps");  \
        return m_Variable->m_AvailableStepsCount;                              \
    }                                                                          \
                                                                               \
    template <>                                                                \
    size_t Variable<T>::StepsStart() const                                     \
    {                                                                          \
        helper::CheckForNullptr(m_Variable,                                    \
                                "in call to Variable<T>::StepsStart");         \
        return m_Variable->m_AvailableStepsStart;                              \
    }                                                                          \
                                                                               \
    template <>                                                                \
    size_t Variable<T>::BlockID() const                                        \
    {                                                                          \
        helper::CheckForNullptr(m_Variable,                                    \
                                "in call to Variable<T>::BlockID");            \
        return m_Variable->m_BlockID;                                          \
    }                                                                          \
                                                                               \
    template <>                                                                \
    size_t Variable<T>::AddOperation(const Operator op,                        \
                                     const Params &parameters)                 \
    {                                                                          \
        helper::CheckForNullptr(m_Variable,                                    \
                                "in call to Variable<T>::AddOperator");        \
        if (!op)                                                               \
        {                                                                      \
            throw std::invalid_argument("ERROR: invalid operator, in call to " \
                                        "Variable<T>::AddOperator");           \
        }                                                                      \
        return m_Variable->AddOperation(*op.m_Operator, parameters);           \
    }                                                                          \
                                                                               \
    template <>                                                                \
    std::vector<typename Variable<T>::Operation> Variable<T>::Operations()     \
        const                                                                  \
    {                                                                          \
        helper::CheckForNullptr(m_Variable,                                    \
                                "in call to Variable<T>::Operations");         \
        std::vector<Operation> operations;                                     \
        operations.reserve(m_Variable->m_Operations.size());                   \
                                                                               \
        for (const auto &op : m_Variable->m_Operations)                        \
        {                                                                      \
            operations.push_back(                                              \
                Operation{Operator(op.Op), op.Parameters, op.Info});           \
        }                                                                      \
        return operations;                                                     \
    }                                                                          \
                                                                               \
    template <>                                                                \
    std::pair<T, T> Variable<T>::MinMax(const size_t step) const               \
    {                                                                          \
        helper::CheckForNullptr(m_Variable, "in call to Variable<T>::MinMax"); \
        return m_Variable->MinMax(step);                                       \
    }                                                                          \
                                                                               \
    template <>                                                                \
    T Variable<T>::Min(const size_t step) const                                \
    {                                                                          \
        helper::CheckForNullptr(m_Variable, "in call to Variable<T>::Min");    \
        return m_Variable->Min(step);                                          \
    }                                                                          \
                                                                               \
    template <>                                                                \
    T Variable<T>::Max(const size_t step) const                                \
    {                                                                          \
        helper::CheckForNullptr(m_Variable, "in call to Variable<T>::Max");    \
        return m_Variable->Max(step);                                          \
    }                                                                          \
                                                                               \
    template <>                                                                \
    std::vector<std::vector<typename Variable<T>::Info>>                       \
    Variable<T>::AllStepsBlocksInfo()                                          \
    {                                                                          \
        return DoAllStepsBlocksInfo();                                         \
    }                                                                          \
                                                                               \
    template <>                                                                \
    const T *Variable<T>::Info::Data() const                                   \
    {                                                                          \
        const core::Variable<T>::Info *coreInfo =                              \
            reinterpret_cast<const core::Variable<T>::Info *>(m_Info);         \
                                                                               \
        return m_Info ? (coreInfo->BufferP ? coreInfo->BufferP                 \
                                           : coreInfo->BufferV.data())         \
                      : nullptr;                                               \
    }

ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

} // end namespace adios2
