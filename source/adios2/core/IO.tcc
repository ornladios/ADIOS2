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
#include "adios2/helper/adiosFunctions.h" //GetType<T>

namespace adios2
{

template <class T>
Variable<T> &IO::DefineVariable(const std::string &name, const Dims &shape,
                                const Dims &start, const Dims &count,
                                const bool constantDims, T *data)
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
    const unsigned int size =
        static_cast<const unsigned int>(variableMap.size());
    auto itVariablePair =
        variableMap.emplace(size, Variable<T>(name, shape, start, count,
                                              constantDims, data, m_DebugMode));
    m_Variables.emplace(name, std::make_pair(GetType<T>(), size));
    return itVariablePair.first->second;
}

template <class T>
Variable<T> *IO::InquireVariable(const std::string &name) noexcept
{
    auto itVariable = m_Variables.find(name);

    if (itVariable == m_Variables.end())
    {
        return nullptr;
    }

    if (itVariable->second.first != GetType<T>())
    {
        return nullptr;
    }

    return &GetVariableMap<T>().at(itVariable->second.second);
}

template <class T>
Attribute<T> &IO::DefineAttribute(const std::string &name, const T &value)
{
    if (m_DebugMode)
    {
        CheckAttributeCommon(name);
    }

    auto &attributeMap = GetAttributeMap<T>();
    const unsigned int size =
        static_cast<const unsigned int>(attributeMap.size());

    auto itAttributePair =
        attributeMap.emplace(size, Attribute<T>(name, value));
    m_Attributes.emplace(name, std::make_pair(GetType<T>(), size));

    return itAttributePair.first->second;
}

template <class T>
Attribute<T> &IO::DefineAttribute(const std::string &name, const T *array,
                                  const size_t elements)
{
    if (m_DebugMode)
    {
        CheckAttributeCommon(name);
    }

    auto &attributeMap = GetAttributeMap<T>();
    const unsigned int size =
        static_cast<const unsigned int>(attributeMap.size());

    auto itAttributePair =
        attributeMap.emplace(size, Attribute<T>(name, array, elements));
    m_Attributes.emplace(name, std::make_pair(GetType<T>(), size));

    return itAttributePair.first->second;
}

template <class T>
Attribute<T> *IO::InquireAttribute(const std::string &name) noexcept
{
    auto itAttribute = m_Attributes.find(name);

    if (itAttribute == m_Attributes.end())
    {
        return nullptr;
    }

    if (itAttribute->second.first != GetType<T>())
    {
        return nullptr;
    }

    return &GetAttributeMap<T>().at(itAttribute->second.second);
}

// PRIVATE
template <>
std::map<unsigned int, Variable<char>> &IO::GetVariableMap()
{
    return m_Char;
}

template <>
std::map<unsigned int, Variable<signed char>> &IO::GetVariableMap()
{
    return m_SChar;
}

template <>
std::map<unsigned int, Variable<unsigned char>> &IO::GetVariableMap()
{
    return m_UChar;
}

template <>
std::map<unsigned int, Variable<short>> &IO::GetVariableMap()
{
    return m_Short;
}

template <>
std::map<unsigned int, Variable<unsigned short>> &IO::GetVariableMap()
{
    return m_UShort;
}

template <>
std::map<unsigned int, Variable<int>> &IO::GetVariableMap()
{
    return m_Int;
}

template <>
std::map<unsigned int, Variable<unsigned int>> &IO::GetVariableMap()
{
    return m_UInt;
}

template <>
std::map<unsigned int, Variable<long int>> &IO::GetVariableMap()
{
    return m_LInt;
}

template <>
std::map<unsigned int, Variable<unsigned long int>> &IO::GetVariableMap()
{
    return m_ULInt;
}

template <>
std::map<unsigned int, Variable<long long int>> &IO::GetVariableMap()
{
    return m_LLInt;
}

template <>
std::map<unsigned int, Variable<unsigned long long int>> &IO::GetVariableMap()
{
    return m_ULLInt;
}

template <>
std::map<unsigned int, Variable<float>> &IO::GetVariableMap()
{
    return m_Float;
}

template <>
std::map<unsigned int, Variable<double>> &IO::GetVariableMap()
{
    return m_Double;
}

template <>
std::map<unsigned int, Variable<long double>> &IO::GetVariableMap()
{
    return m_LDouble;
}

template <>
std::map<unsigned int, Variable<cfloat>> &IO::GetVariableMap()
{
    return m_CFloat;
}

template <>
std::map<unsigned int, Variable<cdouble>> &IO::GetVariableMap()
{
    return m_CDouble;
}

template <>
std::map<unsigned int, Variable<cldouble>> &IO::GetVariableMap()
{
    return m_CLDouble;
}

// attributes
template <>
std::map<unsigned int, Attribute<std::string>> &IO::GetAttributeMap()
{
    return m_StringA;
}

template <>
std::map<unsigned int, Attribute<char>> &IO::GetAttributeMap()
{
    return m_CharA;
}

template <>
std::map<unsigned int, Attribute<signed char>> &IO::GetAttributeMap()
{
    return m_SCharA;
}

template <>
std::map<unsigned int, Attribute<unsigned char>> &IO::GetAttributeMap()
{
    return m_UCharA;
}

template <>
std::map<unsigned int, Attribute<short>> &IO::GetAttributeMap()
{
    return m_ShortA;
}

template <>
std::map<unsigned int, Attribute<unsigned short>> &IO::GetAttributeMap()
{
    return m_UShortA;
}

template <>
std::map<unsigned int, Attribute<int>> &IO::GetAttributeMap()
{
    return m_IntA;
}

template <>
std::map<unsigned int, Attribute<unsigned int>> &IO::GetAttributeMap()
{
    return m_UIntA;
}

template <>
std::map<unsigned int, Attribute<long int>> &IO::GetAttributeMap()
{
    return m_LIntA;
}

template <>
std::map<unsigned int, Attribute<unsigned long int>> &IO::GetAttributeMap()
{
    return m_ULIntA;
}

template <>
std::map<unsigned int, Attribute<long long int>> &IO::GetAttributeMap()
{
    return m_LLIntA;
}

template <>
std::map<unsigned int, Attribute<unsigned long long int>> &IO::GetAttributeMap()
{
    return m_ULLIntA;
}

template <>
std::map<unsigned int, Attribute<float>> &IO::GetAttributeMap()
{
    return m_FloatA;
}

template <>
std::map<unsigned int, Attribute<double>> &IO::GetAttributeMap()
{
    return m_DoubleA;
}

template <>
std::map<unsigned int, Attribute<long double>> &IO::GetAttributeMap()
{
    return m_LDoubleA;
}

} // end namespace adios2

#endif /* ADIOS2_CORE_IO_TCC_ */
