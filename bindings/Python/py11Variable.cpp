/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * py11Variable.cpp :
 *
 *  Created on: Sep 7, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "py11Variable.h"

#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace py11
{

Variable::Variable(core::VariableBase *variable) : m_Variable(variable) {}

Variable::operator bool() const noexcept
{
    return (m_Variable == nullptr) ? false : true;
}

void Variable::SetShape(const Dims &shape)
{
    helper::CheckForNullptr(m_Variable, "in call to Variable::SetShape");
    m_Variable->SetShape(shape);
}

void Variable::SetBlockSelection(const size_t blockID)
{
    helper::CheckForNullptr(m_Variable,
                            "in call to Variable::SetBlockSelection");
    m_Variable->SetBlockSelection(blockID);
}

void Variable::SetSelection(const Box<Dims> &selection)
{
    helper::CheckForNullptr(m_Variable, "in call to Variable::SetSelection");
    m_Variable->SetSelection(selection);
}

void Variable::SetStepSelection(const Box<size_t> &stepSelection)
{
    helper::CheckForNullptr(m_Variable,
                            "in call to Variable::SetStepSelection");
    m_Variable->SetStepSelection(stepSelection);
}

size_t Variable::SelectionSize() const
{
    helper::CheckForNullptr(m_Variable, "in call to Variable::SelectionSize");
    return m_Variable->SelectionSize();
}

size_t Variable::AddOperation(const Operator op, const Params &parameters)
{
    helper::CheckForNullptr(m_Variable, "in call to Variable::AddOperation");
    return m_Variable->AddOperation(*op.m_Operator, parameters);
}

std::vector<Variable::Operation> Variable::Operations() const
{
    helper::CheckForNullptr(m_Variable, "in call to Variable::Operations");
    std::vector<Variable::Operation> operations;
    operations.reserve(m_Variable->m_Operations.size());

    for (const auto &op : m_Variable->m_Operations)
    {
        operations.push_back(
            Operation{Operator(op.Op), op.Parameters, op.Info});
    }
    return operations;
}

std::string Variable::Name() const
{
    helper::CheckForNullptr(m_Variable, "in call to Variable::Name");
    return m_Variable->m_Name;
}

std::string Variable::Type() const
{
    helper::CheckForNullptr(m_Variable, "in call to Variable::Type");
    return m_Variable->m_Type.ToString();
}

size_t Variable::Sizeof() const
{
    helper::CheckForNullptr(m_Variable, "in call to Variable::Sizeof");
    return m_Variable->m_ElementSize;
}

adios2::ShapeID Variable::ShapeID() const
{
    helper::CheckForNullptr(m_Variable, "in call to Variable::ShapeID");
    return m_Variable->m_ShapeID;
}

Dims Variable::Shape() const
{
    helper::CheckForNullptr(m_Variable, "in call to Variable::Shape");
    return m_Variable->m_Shape;
}

Dims Variable::Start() const
{
    helper::CheckForNullptr(m_Variable, "in call to Variable::Start");
    return m_Variable->m_Start;
}

Dims Variable::Count() const
{
    helper::CheckForNullptr(m_Variable, "in call to Variable::Count");
    return m_Variable->m_Count;
}

size_t Variable::Steps() const
{
    helper::CheckForNullptr(m_Variable, "in call to Variable::Steps");
    return m_Variable->m_StepsCount;
}

size_t Variable::StepsStart() const
{
    helper::CheckForNullptr(m_Variable, "in call to Variable::StepsStart");
    return m_Variable->m_StepsStart;
}

size_t Variable::BlockID() const
{
    helper::CheckForNullptr(m_Variable, "in call to Variable::BlockID");
    return m_Variable->m_BlockID;
}

// size_t Variable::AddOperation(const Operator op, const Params &parameters) {}

// std::vector<Operation> Variable::Operations() const {}

} // end namespace py11
} // end namespace adios2
