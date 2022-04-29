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

StructDefinition::StructDefinition(core::StructDefinition *ptr)
: m_StructDefinition(ptr)
{
}

void StructDefinition::AddItem(const std::string &name, const size_t offset,
                               const DataType type, const size_t size)
{
    helper::CheckForNullptr(m_StructDefinition,
                            "in call to StructDefinition::AddItem");
    m_StructDefinition->AddItem(name, offset, type, size);
}

size_t StructDefinition::StructSize() const noexcept
{
    helper::CheckForNullptr(m_StructDefinition,
                            "in call to StructDefinition::StructSize");
    return m_StructDefinition->StructSize();
}

size_t StructDefinition::Items() const noexcept
{
    helper::CheckForNullptr(m_StructDefinition,
                            "in call to StructDefinition::Items");
    return m_StructDefinition->Items();
}
std::string StructDefinition::Name(const size_t index) const
{
    helper::CheckForNullptr(m_StructDefinition,
                            "in call to StructDefinition::Name");
    return m_StructDefinition->Name(index);
}
size_t StructDefinition::Offset(const size_t index) const
{
    helper::CheckForNullptr(m_StructDefinition,
                            "in call to StructDefinition::Offset");
    return m_StructDefinition->Offset(index);
}
DataType StructDefinition::Type(const size_t index) const
{
    helper::CheckForNullptr(m_StructDefinition,
                            "in call to StructDefinition::Type");
    return m_StructDefinition->Type(index);
}
size_t StructDefinition::Size(const size_t index) const
{
    helper::CheckForNullptr(m_StructDefinition,
                            "in call to StructDefinition::Size");
    return m_StructDefinition->Size(index);
}

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
    auto type = m_Variable->m_Type;
#define declare_type(T)                                                        \
    if (type == helper::GetDataType<T>())                                      \
    {                                                                          \
        return reinterpret_cast<core::Variable<T> *>(m_Variable)->Shape(step); \
    }
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
    else if (type == DataType::Struct)
    {
        return reinterpret_cast<core::VariableStruct *>(m_Variable)->m_Shape;
    }
    helper::Throw<std::runtime_error>("bindings::CXX11", "VariableNT", "Shape",
                                      "invalid data type " + ToString(type));
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
    auto type = m_Variable->m_Type;
#define declare_type(T)                                                        \
    if (type == helper::GetDataType<T>())                                      \
    {                                                                          \
        return reinterpret_cast<core::Variable<T> *>(m_Variable)->Count();     \
    }
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
    else if (type == DataType::Struct)
    {
        return reinterpret_cast<core::VariableStruct *>(m_Variable)->m_Count;
    }
    helper::Throw<std::runtime_error>("bindings::CXX11", "VariableNT", "Count",
                                      "invalid data type " + ToString(type));
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

void VariableNT::RemoveOperations()
{
    helper::CheckForNullptr(m_Variable,
                            "in call to VariableNT::RemoveOperations");
    m_Variable->RemoveOperations();
}

size_t VariableNT::StructItems() const
{
    helper::CheckForNullptr(m_Variable, "in call to VariableNT::StructItems");
    if (m_Variable->m_Type != DataType::Struct)
    {
        helper::Throw<std::runtime_error>(
            "bindings::CXX11", "VariableNT", "StructItems",
            "invalid data type " + ToString(m_Variable->m_Type));
    }
    return reinterpret_cast<core::VariableStruct *>(m_Variable)
        ->m_StructDefinition.Items();
}
std::string VariableNT::StructItemName(const size_t index) const
{
    helper::CheckForNullptr(m_Variable,
                            "in call to VariableNT::StructItemName");
    if (m_Variable->m_Type != DataType::Struct)
    {
        helper::Throw<std::runtime_error>(
            "bindings::CXX11", "VariableNT", "StructItemName",
            "invalid data type " + ToString(m_Variable->m_Type));
    }
    return reinterpret_cast<core::VariableStruct *>(m_Variable)
        ->m_StructDefinition.Name(index);
}
size_t VariableNT::StructItemOffset(const size_t index) const
{
    helper::CheckForNullptr(m_Variable,
                            "in call to VariableNT::StructItemOffset");
    if (m_Variable->m_Type != DataType::Struct)
    {
        helper::Throw<std::runtime_error>(
            "bindings::CXX11", "VariableNT", "StructItemOffset",
            "invalid data type " + ToString(m_Variable->m_Type));
    }
    return reinterpret_cast<core::VariableStruct *>(m_Variable)
        ->m_StructDefinition.Offset(index);
}
DataType VariableNT::StructItemType(const size_t index) const
{
    helper::CheckForNullptr(m_Variable,
                            "in call to VariableNT::StructItemType");
    if (m_Variable->m_Type != DataType::Struct)
    {
        helper::Throw<std::runtime_error>(
            "bindings::CXX11", "VariableNT", "StructItemType",
            "invalid data type " + ToString(m_Variable->m_Type));
    }
    return reinterpret_cast<core::VariableStruct *>(m_Variable)
        ->m_StructDefinition.Type(index);
}
size_t VariableNT::StructItemSize(const size_t index) const
{
    helper::CheckForNullptr(m_Variable,
                            "in call to VariableNT::StructItemSize");
    if (m_Variable->m_Type != DataType::Struct)
    {
        helper::Throw<std::runtime_error>(
            "bindings::CXX11", "VariableNT", "StructItemSize",
            "invalid data type " + ToString(m_Variable->m_Type));
    }
    return reinterpret_cast<core::VariableStruct *>(m_Variable)
        ->m_StructDefinition.Size(index);
}

} // end namespace adios2
