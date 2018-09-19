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

#include "adios2/ADIOSMPI.h"
#include "adios2/ADIOSMacros.h"
#include "adios2/helper/adiosFunctions.h" //helper::GetType<T>

namespace adios2
{
namespace core
{

template <class T>
Variable<T> &IO::DefineVariable(const std::string &name, const Dims &shape,
                                const Dims &start, const Dims &count,
                                const bool constantDims)
{
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
    const unsigned int size = static_cast<unsigned int>(variableMap.size());
    auto itVariablePair =
        variableMap.emplace(size, Variable<T>(name, shape, start, count,
                                              constantDims, m_DebugMode));
    m_Variables.emplace(name, std::make_pair(helper::GetType<T>(), size));

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
    if (m_Streaming)
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
    const std::string globalName =
        helper::GlobalName(name, variableName, separator);
    if (m_DebugMode)
    {
        CheckAttributeCommon(globalName);
    }

    auto &attributeMap = GetAttributeMap<T>();
    const unsigned int size = static_cast<unsigned int>(attributeMap.size());

    auto itAttributePair =
        attributeMap.emplace(size, Attribute<T>(globalName, value));
    m_Attributes.emplace(globalName,
                         std::make_pair(helper::GetType<T>(), size));

    return itAttributePair.first->second;
}

template <class T>
Attribute<T> &IO::DefineAttribute(const std::string &name, const T *array,
                                  const size_t elements,
                                  const std::string &variableName,
                                  const std::string separator)
{
    const std::string globalName =
        helper::GlobalName(name, variableName, separator);

    if (m_DebugMode)
    {
        CheckAttributeCommon(globalName);
    }

    auto &attributeMap = GetAttributeMap<T>();
    const unsigned int size = static_cast<unsigned int>(attributeMap.size());

    auto itAttributePair =
        attributeMap.emplace(size, Attribute<T>(globalName, array, elements));
    m_Attributes.emplace(globalName,
                         std::make_pair(helper::GetType<T>(), size));

    return itAttributePair.first->second;
}

template <class T>
Attribute<T> *IO::InquireAttribute(const std::string &name,
                                   const std::string &variableName,
                                   const std::string separator) noexcept
{
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
template <>
std::map<unsigned int, Variable<std::string>> &IO::GetVariableMap() noexcept
{
    return m_String;
}

template <>
std::map<unsigned int, Variable<char>> &IO::GetVariableMap() noexcept
{
    return m_Char;
}

template <>
std::map<unsigned int, Variable<signed char>> &IO::GetVariableMap() noexcept
{
    return m_SChar;
}

template <>
std::map<unsigned int, Variable<unsigned char>> &IO::GetVariableMap() noexcept
{
    return m_UChar;
}

template <>
std::map<unsigned int, Variable<short>> &IO::GetVariableMap() noexcept
{
    return m_Short;
}

template <>
std::map<unsigned int, Variable<unsigned short>> &IO::GetVariableMap() noexcept
{
    return m_UShort;
}

template <>
std::map<unsigned int, Variable<int>> &IO::GetVariableMap() noexcept
{
    return m_Int;
}

template <>
std::map<unsigned int, Variable<unsigned int>> &IO::GetVariableMap() noexcept
{
    return m_UInt;
}

template <>
std::map<unsigned int, Variable<long int>> &IO::GetVariableMap() noexcept
{
    return m_LInt;
}

template <>
std::map<unsigned int, Variable<unsigned long int>> &
IO::GetVariableMap() noexcept
{
    return m_ULInt;
}

template <>
std::map<unsigned int, Variable<long long int>> &IO::GetVariableMap() noexcept
{
    return m_LLInt;
}

template <>
std::map<unsigned int, Variable<unsigned long long int>> &
IO::GetVariableMap() noexcept
{
    return m_ULLInt;
}

template <>
std::map<unsigned int, Variable<float>> &IO::GetVariableMap() noexcept
{
    return m_Float;
}

template <>
std::map<unsigned int, Variable<double>> &IO::GetVariableMap() noexcept
{
    return m_Double;
}

template <>
std::map<unsigned int, Variable<long double>> &IO::GetVariableMap() noexcept
{
    return m_LDouble;
}

template <>
std::map<unsigned int, Variable<cfloat>> &IO::GetVariableMap() noexcept
{
    return m_CFloat;
}

template <>
std::map<unsigned int, Variable<cdouble>> &IO::GetVariableMap() noexcept
{
    return m_CDouble;
}

// attributes
template <>
std::map<unsigned int, Attribute<std::string>> &IO::GetAttributeMap() noexcept
{
    return m_StringA;
}

template <>
std::map<unsigned int, Attribute<char>> &IO::GetAttributeMap() noexcept
{
    return m_CharA;
}

template <>
std::map<unsigned int, Attribute<signed char>> &IO::GetAttributeMap() noexcept
{
    return m_SCharA;
}

template <>
std::map<unsigned int, Attribute<unsigned char>> &IO::GetAttributeMap() noexcept
{
    return m_UCharA;
}

template <>
std::map<unsigned int, Attribute<short>> &IO::GetAttributeMap() noexcept
{
    return m_ShortA;
}

template <>
std::map<unsigned int, Attribute<unsigned short>> &
IO::GetAttributeMap() noexcept
{
    return m_UShortA;
}

template <>
std::map<unsigned int, Attribute<int>> &IO::GetAttributeMap() noexcept
{
    return m_IntA;
}

template <>
std::map<unsigned int, Attribute<unsigned int>> &IO::GetAttributeMap() noexcept
{
    return m_UIntA;
}

template <>
std::map<unsigned int, Attribute<long int>> &IO::GetAttributeMap() noexcept
{
    return m_LIntA;
}

template <>
std::map<unsigned int, Attribute<unsigned long int>> &
IO::GetAttributeMap() noexcept
{
    return m_ULIntA;
}

template <>
std::map<unsigned int, Attribute<long long int>> &IO::GetAttributeMap() noexcept
{
    return m_LLIntA;
}

template <>
std::map<unsigned int, Attribute<unsigned long long int>> &
IO::GetAttributeMap() noexcept
{
    return m_ULLIntA;
}

template <>
std::map<unsigned int, Attribute<float>> &IO::GetAttributeMap() noexcept
{
    return m_FloatA;
}

template <>
std::map<unsigned int, Attribute<double>> &IO::GetAttributeMap() noexcept
{
    return m_DoubleA;
}

template <>
std::map<unsigned int, Attribute<long double>> &IO::GetAttributeMap() noexcept
{
    return m_LDoubleA;
}

} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_CORE_IO_TCC_ */
