/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Variable.cpp :
 *
 *  Created on: Apr 18, 2022
 *      Author: Jason Wang jason.ruonan.wang@gmail.com
 */

#include "VariableNT.h"
#include "Types.h"
#include "adios2/core/VariableBase.h"
#include "adios2/helper/adiosFunctions.h"

namespace adios2
{

VariableNT::VariableNT(core::VariableBase *variable) : m_Variable(variable) {}

VariableNT::operator bool() const noexcept { return m_Variable != nullptr; }

void VariableNT::SetMemorySpace(const MemorySpace mem)
{
    helper::CheckForNullptr(m_Variable,
                            "in call to VariableNT::SetMemorySpace");
    m_Variable->SetMemorySpace(mem);
}

void VariableNT::SetShape(const Dims &shape)
{
    helper::CheckForNullptr(m_Variable, "in call to VariableNT::SetShape");
    m_Variable->SetShape(shape);
}

void VariableNT::SetBlockSelection(const size_t blockID)
{
    helper::CheckForNullptr(m_Variable,
                            "in call to VariableNT::SetBlockSelection");
    m_Variable->SetBlockSelection(blockID);
}

void VariableNT::SetSelection(const Box<Dims> &selection)
{
    helper::CheckForNullptr(m_Variable, "in call to VariableNT::SetSelection");
    m_Variable->SetSelection(selection);
}

void VariableNT::SetMemorySelection(const Box<Dims> &memorySelection)
{
    helper::CheckForNullptr(m_Variable,
                            "in call to VariableNT::SetMemorySelection");
    m_Variable->SetMemorySelection(memorySelection);
}

void VariableNT::SetStepSelection(const Box<size_t> &stepSelection)
{
    helper::CheckForNullptr(m_Variable,
                            "in call to VariableNT::SetStepSelection");
    m_Variable->SetStepSelection(stepSelection);
}

size_t VariableNT::SelectionSize() const
{
    helper::CheckForNullptr(m_Variable, "in call to VariableNT::SelectionSize");
    auto type = ToString(m_Variable->m_Type);
#define declare_type(T)                                                        \
    if (type == GetType<T>())                                                  \
    {                                                                          \
        return reinterpret_cast<core::Variable<T> *>(m_Variable)               \
            ->SelectionSize();                                                 \
    }
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
    helper::Throw<std::runtime_error>("bindings::CXX11", "VariableNT",
                                      "SelectionSize",
                                      "invalid data type " + type);
    return 0;
}

std::string VariableNT::Name() const
{
    helper::CheckForNullptr(m_Variable, "in call to VariableNT::Name");
    return m_Variable->m_Name;
}

std::string VariableNT::Type() const
{
    helper::CheckForNullptr(m_Variable, "in call to VariableNT::Type");
    return ToString(m_Variable->m_Type);
}

size_t VariableNT::Sizeof() const
{
    helper::CheckForNullptr(m_Variable, "in call to VariableNT::Sizeof");
    return m_Variable->m_ElementSize;
}

adios2::ShapeID VariableNT::ShapeID() const
{
    helper::CheckForNullptr(m_Variable, "in call to VariableNT::ShapeID");
    return m_Variable->m_ShapeID;
}

Dims VariableNT::Shape(const size_t step) const
{
    helper::CheckForNullptr(m_Variable, "in call to VariableNT::Shape");
    auto type = ToString(m_Variable->m_Type);
#define declare_type(T)                                                        \
    if (type == GetType<T>())                                                  \
    {                                                                          \
        return reinterpret_cast<core::Variable<T> *>(m_Variable)->Shape(step); \
    }
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
    helper::Throw<std::runtime_error>("bindings::CXX11", "VariableNT", "Shape",
                                      "invalid data type " + type);
    return Dims();
}

Dims VariableNT::Start() const
{
    helper::CheckForNullptr(m_Variable, "in call to VariableNT::Start");
    return m_Variable->m_Start;
}

Dims VariableNT::Count() const
{
    helper::CheckForNullptr(m_Variable, "in call to VariableNT::Count");
    auto type = ToString(m_Variable->m_Type);
#define declare_type(T)                                                        \
    if (type == GetType<T>())                                                  \
    {                                                                          \
        return reinterpret_cast<core::Variable<T> *>(m_Variable)->Count();     \
    }
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
    helper::Throw<std::runtime_error>("bindings::CXX11", "VariableNT", "Count",
                                      "invalid data type " + type);
    return Dims();
}

size_t VariableNT::Steps() const
{
    helper::CheckForNullptr(m_Variable, "in call to VariableNT::Steps");
    return m_Variable->m_AvailableStepsCount;
}

size_t VariableNT::StepsStart() const
{
    helper::CheckForNullptr(m_Variable, "in call to VariableNT::StepsStart");
    return m_Variable->m_AvailableStepsStart;
}

size_t VariableNT::BlockID() const
{
    helper::CheckForNullptr(m_Variable, "in call to VariableNT::BlockID");
    return m_Variable->m_BlockID;
}

size_t VariableNT::AddOperation(const Operator op, const Params &parameters)
{
    helper::CheckForNullptr(m_Variable, "in call to VariableNT::AddOperation");
    if (!op)
    {
        helper::Throw<std::invalid_argument>("bindings::CXX11", "VariableNT",
                                             "AddOperation",
                                             "invalid operation");
    }
    auto params = op.Parameters();
    for (const auto &p : parameters)
    {
        params[p.first] = p.second;
    }
    return m_Variable->AddOperation(op.m_Type, params);
}

size_t VariableNT::AddOperation(const std::string &type,
                                const Params &parameters)
{
    helper::CheckForNullptr(m_Variable, "in call to VariableNT::AddOperation");
    return m_Variable->AddOperation(type, parameters);
}

std::vector<Operator> VariableNT::Operations() const
{
    helper::CheckForNullptr(m_Variable, "in call to VariableNT::Operations");
    std::vector<Operator> operations;
    operations.reserve(m_Variable->m_Operations.size());
    for (const auto &op : m_Variable->m_Operations)
    {
        operations.push_back(Operator(op->m_TypeString, &op->GetParameters()));
    }
    return operations;
}

void VariableNT::AddSubVariable(const std::string &name, const DataType type,
                                const size_t size)
{
    helper::CheckForNullptr(m_Variable,
                            "in call to VariableNT::AddSubVariable");
    if (m_Variable->m_Type == DataType::Struct)
    {
        reinterpret_cast<core::VariableStruct *>(m_Variable)
            ->AddSubVariable(name, type, size);
    }
    else
    {
        helper::Throw<std::runtime_error>(
            "bindings::CXX11", "VariableNT", "AddSubVariable",
            "non-struct variable does not support AddSubVariable");
    }
}

void VariableNT::RemoveOperations()
{
    helper::CheckForNullptr(m_Variable,
                            "in call to VariableNT::RemoveOperations");
    m_Variable->RemoveOperations();
}

} // end namespace adios2
