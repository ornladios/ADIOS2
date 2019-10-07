/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * IO.tcc template implementations with fix types and specializations
 *
 *  Created on: May 15, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_CORE_IO_TCC_
#define ADIOS2_CORE_IO_TCC_

#include "IO.h"

/// \cond EXCLUDE_FROM_DOXYGEN
#include <iostream>
#include <stdexcept> //std::invalid_argument
/// \endcond

#include "adios2/common/ADIOSMacros.h"
#include "adios2/helper/adiosFunctions.h" //helper::GetType<T>
#include "adios2/toolkit/profiling/taustubs/tautimer.hpp"

namespace adios2
{
namespace core
{

template <class T>
Variable<T> &IO::DefineVariable(const std::string &name, const Dims &shape,
                                const Dims &start, const Dims &count,
                                const bool constantDims)
{
    TAU_SCOPED_TIMER("IO::DefineVariable");
    if (m_DebugMode)
    {
        auto itVariable = m_Variables.find(name);
        if (!IsEnd(itVariable, m_Variables))
        {
            throw std::invalid_argument("ERROR: variable " + name +
                                        " exists in IO object " + m_Name +
                                        ", in call to DefineVariable\n");
        }
    }

    auto &variableMap = GetVariableMap<T>();
    const unsigned int newIndex =
        variableMap.empty() ? 0 : variableMap.rbegin()->first + 1;

    auto itVariablePair =
        variableMap.emplace(newIndex, Variable<T>(name, shape, start, count,
                                                  constantDims, m_DebugMode));
    m_Variables.emplace(name, std::make_pair(helper::GetType<T>(), newIndex));

    Variable<T> &variable = itVariablePair.first->second;

    // check IO placeholder for variable operations
    auto itOperations = m_VarOpsPlaceholder.find(name);
    if (itOperations != m_VarOpsPlaceholder.end())
    {
        variable.m_Operations.reserve(itOperations->second.size());

        for (auto &operation : itOperations->second)
        {
            variable.AddOperation(*operation.Op, operation.Parameters);
        }
    }

    return variable;
}

template <class T>
Variable<T> *IO::InquireVariable(const std::string &name) noexcept
{
    TAU_SCOPED_TIMER("IO::InquireVariable");
    auto itVariable = m_Variables.find(name);

    if (itVariable == m_Variables.end())
    {
        return nullptr;
    }

    if (itVariable->second.first != helper::GetType<T>())
    {
        return nullptr;
    }

    Variable<T> *variable = &GetVariableMap<T>().at(itVariable->second.second);
    if (m_ReadStreaming)
    {
        if (!variable->IsValidStep(m_EngineStep + 1))
        {
            return nullptr;
        }
    }
    return variable;
}

template <class T>
Attribute<T> &IO::DefineAttribute(const std::string &name, const T &value,
                                  const std::string &variableName,
                                  const std::string separator)
{
    TAU_SCOPED_TIMER("IO::DefineAttribute");
    if (m_DebugMode)
    {
        if (!variableName.empty() && InquireVariableType(variableName).empty())
        {
            throw std::invalid_argument(
                "ERROR: variable " + variableName +
                " doesn't exist, can't associate attribute " + name +
                ", in call to DefineAttribute");
        }
    }

    const std::string globalName =
        helper::GlobalName(name, variableName, separator);
    if (m_DebugMode)
    {
        CheckAttributeCommon(globalName);
    }

    auto &attributeMap = GetAttributeMap<T>();
    const unsigned int newIndex =
        attributeMap.empty() ? 0 : attributeMap.rbegin()->first + 1;

    auto itAttributePair =
        attributeMap.emplace(newIndex, Attribute<T>(globalName, value));
    m_Attributes.emplace(globalName,
                         std::make_pair(helper::GetType<T>(), newIndex));

    return itAttributePair.first->second;
}

template <class T>
Attribute<T> &IO::DefineAttribute(const std::string &name, const T *array,
                                  const size_t elements,
                                  const std::string &variableName,
                                  const std::string separator)
{
    TAU_SCOPED_TIMER("IO::DefineAttribute");
    if (m_DebugMode)
    {
        if (!variableName.empty() && InquireVariableType(variableName).empty())
        {
            throw std::invalid_argument(
                "ERROR: variable " + variableName +
                " doesn't exist, can't associate attribute " + name +
                ", in call to DefineAttribute");
        }
    }

    const std::string globalName =
        helper::GlobalName(name, variableName, separator);

    if (m_DebugMode)
    {
        CheckAttributeCommon(globalName);
    }

    auto &attributeMap = GetAttributeMap<T>();
    const unsigned int newIndex =
        attributeMap.empty() ? 0 : attributeMap.rbegin()->first + 1;

    auto itAttributePair = attributeMap.emplace(
        newIndex, Attribute<T>(globalName, array, elements));
    m_Attributes.emplace(globalName,
                         std::make_pair(helper::GetType<T>(), newIndex));

    return itAttributePair.first->second;
}

template <class T>
Attribute<T> *IO::InquireAttribute(const std::string &name,
                                   const std::string &variableName,
                                   const std::string separator) noexcept
{
    TAU_SCOPED_TIMER("IO::InquireAttribute");
    const std::string globalName =
        helper::GlobalName(name, variableName, separator);
    auto itAttribute = m_Attributes.find(globalName);

    if (itAttribute == m_Attributes.end())
    {
        return nullptr;
    }

    if (itAttribute->second.first != helper::GetType<T>())
    {
        return nullptr;
    }

    return &GetAttributeMap<T>().at(itAttribute->second.second);
}

// PRIVATE

// GetVariableMap
#define make_GetVariableMap(T, NAME)                                           \
    template <>                                                                \
    std::map<unsigned int, Variable<T>> &IO::GetVariableMap() noexcept         \
    {                                                                          \
        return m_##NAME;                                                       \
    }
ADIOS2_FOREACH_STDTYPE_2ARGS(make_GetVariableMap)
#undef make_GetVariableMap

// GetAttributeMap
#define make_GetAttributeMap(T, NAME)                                          \
    template <>                                                                \
    std::map<unsigned int, Attribute<T>> &IO::GetAttributeMap() noexcept       \
    {                                                                          \
        return m_##NAME##A;                                                    \
    }
ADIOS2_FOREACH_ATTRIBUTE_STDTYPE_2ARGS(make_GetAttributeMap)
#undef make_GetAttributeMap

} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_CORE_IO_TCC_ */
