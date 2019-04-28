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
    Variable<T>::Variable(const std::string &name, core::IO *io)               \
    : m_CoreIO(io), m_Name(name)                                               \
    {                                                                          \
    }                                                                          \
                                                                               \
    template <>                                                                \
    core::Variable<Variable<T>::IOType> *Variable<T>::CoreVariable()           \
    {                                                                          \
        if (!m_CoreIO)                                                         \
        {                                                                      \
            return nullptr;                                                    \
        }                                                                      \
        return m_CoreIO->InquireVariable<IOType>(m_Name);                      \
    }                                                                          \
                                                                               \
    template <>                                                                \
    const core::Variable<Variable<T>::IOType> *Variable<T>::CoreVariable()     \
        const                                                                  \
    {                                                                          \
        if (!m_CoreIO)                                                         \
        {                                                                      \
            return nullptr;                                                    \
        }                                                                      \
        return m_CoreIO->InquireVariable<IOType>(m_Name);                      \
    }                                                                          \
                                                                               \
    template <>                                                                \
    Variable<T>::operator bool() const noexcept                                \
    {                                                                          \
        return (CoreVariable() == nullptr) ? false : true;                     \
    }                                                                          \
                                                                               \
    template <>                                                                \
    void Variable<T>::SetShape(const Dims &shape)                              \
    {                                                                          \
        helper::CheckForNullptr(CoreVariable(),                                \
                                "in call to Variable<T>::SetShape");           \
        CoreVariable()->SetShape(shape);                                       \
    }                                                                          \
                                                                               \
    template <>                                                                \
    void Variable<T>::SetBlockSelection(const size_t blockID)                  \
    {                                                                          \
        helper::CheckForNullptr(CoreVariable(),                                \
                                "in call to Variable<T>::SetBlockSelection");  \
        CoreVariable()->SetBlockSelection(blockID);                            \
    }                                                                          \
                                                                               \
    template <>                                                                \
    void Variable<T>::SetSelection(const Box<Dims> &selection)                 \
    {                                                                          \
        helper::CheckForNullptr(CoreVariable(),                                \
                                "in call to Variable<T>::SetSelection");       \
        CoreVariable()->SetSelection(selection);                               \
    }                                                                          \
                                                                               \
    template <>                                                                \
    void Variable<T>::SetMemorySelection(const Box<Dims> &memorySelection)     \
    {                                                                          \
        helper::CheckForNullptr(CoreVariable(),                                \
                                "in call to Variable<T>::SetMemorySelection"); \
        CoreVariable()->SetMemorySelection(memorySelection);                   \
    }                                                                          \
                                                                               \
    template <>                                                                \
    void Variable<T>::SetStepSelection(const Box<size_t> &stepSelection)       \
    {                                                                          \
        helper::CheckForNullptr(CoreVariable(),                                \
                                "in call to Variable<T>::SetStepSelection");   \
        CoreVariable()->SetStepSelection(stepSelection);                       \
    }                                                                          \
                                                                               \
    template <>                                                                \
    size_t Variable<T>::SelectionSize() const                                  \
    {                                                                          \
        helper::CheckForNullptr(CoreVariable(),                                \
                                "in call to Variable<T>::SelectionSize");      \
        return CoreVariable()->SelectionSize();                                \
    }                                                                          \
                                                                               \
    template <>                                                                \
    std::string Variable<T>::Name() const                                      \
    {                                                                          \
        return m_Name;                                                         \
    }                                                                          \
                                                                               \
    template <>                                                                \
    std::string Variable<T>::Type() const                                      \
    {                                                                          \
        helper::CheckForNullptr(CoreVariable(),                                \
                                "in call to Variable<T>::Type");               \
        return CoreVariable()->m_Type;                                         \
    }                                                                          \
    template <>                                                                \
    size_t Variable<T>::Sizeof() const                                         \
    {                                                                          \
        helper::CheckForNullptr(CoreVariable(),                                \
                                "in call to Variable<T>::Sizeof");             \
        return CoreVariable()->m_ElementSize;                                  \
    }                                                                          \
                                                                               \
    template <>                                                                \
    adios2::ShapeID Variable<T>::ShapeID() const                               \
    {                                                                          \
        helper::CheckForNullptr(CoreVariable(),                                \
                                "in call to Variable<T>::ShapeID");            \
        return CoreVariable()->m_ShapeID;                                      \
    }                                                                          \
                                                                               \
    template <>                                                                \
    Dims Variable<T>::Shape(const size_t step) const                           \
    {                                                                          \
        helper::CheckForNullptr(CoreVariable(),                                \
                                "in call to Variable<T>::Shape");              \
        return CoreVariable()->Shape(step);                                    \
    }                                                                          \
                                                                               \
    template <>                                                                \
    Dims Variable<T>::Start() const                                            \
    {                                                                          \
        helper::CheckForNullptr(CoreVariable(),                                \
                                "in call to Variable<T>::Start");              \
        return CoreVariable()->m_Start;                                        \
    }                                                                          \
                                                                               \
    template <>                                                                \
    Dims Variable<T>::Count() const                                            \
    {                                                                          \
        helper::CheckForNullptr(CoreVariable(),                                \
                                "in call to Variable<T>::Count");              \
        return CoreVariable()->Count();                                        \
    }                                                                          \
                                                                               \
    template <>                                                                \
    size_t Variable<T>::Steps() const                                          \
    {                                                                          \
        helper::CheckForNullptr(CoreVariable(),                                \
                                "in call to Variable<T>::Steps");              \
        return CoreVariable()->m_AvailableStepsCount;                          \
    }                                                                          \
                                                                               \
    template <>                                                                \
    size_t Variable<T>::StepsStart() const                                     \
    {                                                                          \
        helper::CheckForNullptr(CoreVariable(),                                \
                                "in call to Variable<T>::StepsStart");         \
        return CoreVariable()->m_AvailableStepsStart;                          \
    }                                                                          \
                                                                               \
    template <>                                                                \
    size_t Variable<T>::BlockID() const                                        \
    {                                                                          \
        helper::CheckForNullptr(CoreVariable(),                                \
                                "in call to Variable<T>::BlockID");            \
        return CoreVariable()->m_BlockID;                                      \
    }                                                                          \
                                                                               \
    template <>                                                                \
    size_t Variable<T>::AddOperation(const Operator op,                        \
                                     const Params &parameters)                 \
    {                                                                          \
        helper::CheckForNullptr(CoreVariable(),                                \
                                "in call to Variable<T>::AddOperator");        \
        if (!op)                                                               \
        {                                                                      \
            throw std::invalid_argument("ERROR: invalid operator, in call to " \
                                        "Variable<T>::AddOperator");           \
        }                                                                      \
        return CoreVariable()->AddOperation(*op.m_Operator, parameters);       \
    }                                                                          \
                                                                               \
    template <>                                                                \
    std::vector<typename Variable<T>::Operation> Variable<T>::Operations()     \
        const                                                                  \
    {                                                                          \
        helper::CheckForNullptr(CoreVariable(),                                \
                                "in call to Variable<T>::Operations");         \
        std::vector<Operation> operations;                                     \
        operations.reserve(CoreVariable()->m_Operations.size());               \
                                                                               \
        for (const auto &op : CoreVariable()->m_Operations)                    \
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
        helper::CheckForNullptr(CoreVariable(),                                \
                                "in call to Variable<T>::MinMax");             \
        return CoreVariable()->MinMax(step);                                   \
    }                                                                          \
                                                                               \
    template <>                                                                \
    T Variable<T>::Min(const size_t step) const                                \
    {                                                                          \
        helper::CheckForNullptr(CoreVariable(),                                \
                                "in call to Variable<T>::Min");                \
        return CoreVariable()->Min(step);                                      \
    }                                                                          \
                                                                               \
    template <>                                                                \
    T Variable<T>::Max(const size_t step) const                                \
    {                                                                          \
        helper::CheckForNullptr(CoreVariable(),                                \
                                "in call to Variable<T>::Max");                \
        return CoreVariable()->Max(step);                                      \
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

#define declare_type(T)                                                        \
                                                                               \
    template <>                                                                \
    Variable<T>::Span::Span(CoreSpan *coreSpan) : m_Span(coreSpan)             \
    {                                                                          \
    }                                                                          \
                                                                               \
    template <>                                                                \
    size_t Variable<T>::Span::size() const noexcept                            \
    {                                                                          \
        const core::Variable<IOType>::Span *coreSpan =                         \
            reinterpret_cast<const core::Variable<IOType>::Span *>(m_Span);    \
        return coreSpan->Size();                                               \
    }                                                                          \
                                                                               \
    template <>                                                                \
    T *Variable<T>::Span::data() const noexcept                                \
    {                                                                          \
        const core::Variable<IOType>::Span *coreSpan =                         \
            reinterpret_cast<const core::Variable<IOType>::Span *>(m_Span);    \
        return reinterpret_cast<T *>(coreSpan->Data());                        \
    }                                                                          \
                                                                               \
    template <>                                                                \
    T &Variable<T>::Span::at(const size_t position)                            \
    {                                                                          \
        core::Variable<IOType>::Span *coreSpan =                               \
            reinterpret_cast<core::Variable<IOType>::Span *>(m_Span);          \
        IOType &data = coreSpan->At(position);                                 \
        return reinterpret_cast<T &>(data);                                    \
    }                                                                          \
                                                                               \
    template <>                                                                \
    const T &Variable<T>::Span::at(const size_t position) const                \
    {                                                                          \
        const core::Variable<IOType>::Span *coreSpan =                         \
            reinterpret_cast<const core::Variable<IOType>::Span *>(m_Span);    \
        const IOType &data = coreSpan->At(position);                           \
        return reinterpret_cast<const T &>(data);                              \
    }                                                                          \
                                                                               \
    template <>                                                                \
    T &Variable<T>::Span::operator[](const size_t position)                    \
    {                                                                          \
        core::Variable<IOType>::Span *coreSpan =                               \
            reinterpret_cast<core::Variable<IOType>::Span *>(m_Span);          \
        IOType &data = coreSpan->Access(position);                             \
        return reinterpret_cast<T &>(data);                                    \
    }                                                                          \
                                                                               \
    template <>                                                                \
    const T &Variable<T>::Span::operator[](const size_t position) const        \
    {                                                                          \
        const core::Variable<IOType>::Span *coreSpan =                         \
            reinterpret_cast<const core::Variable<IOType>::Span *>(m_Span);    \
        const IOType &data = coreSpan->Access(position);                       \
        return reinterpret_cast<const T &>(data);                              \
    }

ADIOS2_FOREACH_PRIMITIVE_TYPE_1ARG(declare_type)
#undef declare_type

} // end namespace adios2
