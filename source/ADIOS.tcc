/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS.tcc
 *   This contains the template specialization implementations for the ADIOS
 *   class
 */

#ifndef ADIOS_TCC_
#define ADIOS_TCC_

#include <complex>
#include <map>
#include <memory> //std::shared_ptr
#include <ostream>
#include <set>
#include <string>
#include <vector>

#include "ADIOS.h"
#include "ADIOSMacros.h"

namespace adios
{

// -----------------------------------------------------------------------------
// template specializations of GetVarMap helper function
// -----------------------------------------------------------------------------

template <>
std::map<unsigned int, Variable<char>> &ADIOS::GetVarMap()
{
    return m_Char;
}

template <>
std::map<unsigned int, Variable<unsigned char>> &ADIOS::GetVarMap()
{
    return m_UChar;
}

template <>
std::map<unsigned int, Variable<short>> &ADIOS::GetVarMap()
{
    return m_Short;
}

template <>
std::map<unsigned int, Variable<unsigned short>> &ADIOS::GetVarMap()
{
    return m_UShort;
}

template <>
std::map<unsigned int, Variable<int>> &ADIOS::GetVarMap()
{
    return m_Int;
}

template <>
std::map<unsigned int, Variable<unsigned int>> &ADIOS::GetVarMap()
{
    return m_UInt;
}

template <>
std::map<unsigned int, Variable<long int>> &ADIOS::GetVarMap()
{
    return m_LInt;
}

template <>
std::map<unsigned int, Variable<unsigned long int>> &ADIOS::GetVarMap()
{
    return m_ULInt;
}

template <>
std::map<unsigned int, Variable<long long int>> &ADIOS::GetVarMap()
{
    return m_LLInt;
}

template <>
std::map<unsigned int, Variable<unsigned long long int>> &ADIOS::GetVarMap()
{
    return m_ULLInt;
}

template <>
std::map<unsigned int, Variable<float>> &ADIOS::GetVarMap()
{
    return m_Float;
}

template <>
std::map<unsigned int, Variable<double>> &ADIOS::GetVarMap()
{
    return m_Double;
}

template <>
std::map<unsigned int, Variable<long double>> &ADIOS::GetVarMap()
{
    return m_LDouble;
}

template <>
std::map<unsigned int, Variable<std::complex<float>>> &ADIOS::GetVarMap()
{
    return m_CFloat;
}

template <>
std::map<unsigned int, Variable<std::complex<double>>> &ADIOS::GetVarMap()
{
    return m_CDouble;
}

template <>
std::map<unsigned int, Variable<std::complex<long double>>> &ADIOS::GetVarMap()
{
    return m_CLDouble;
}

// -----------------------------------------------------------------------------
// template specializations of DefineVariable:
// -----------------------------------------------------------------------------

template <typename T>
Variable<T> &
ADIOS::DefineVariable(const std::string &name, const Dims dimensions,
                      const Dims globalDimensions, const Dims globalOffsets)
{
    auto &varMap = GetVarMap<T>();
    CheckVariableInput(name, dimensions);
    const unsigned int size = varMap.size();
    varMap.emplace(size, Variable<T>(name, dimensions, globalDimensions,
                                     globalOffsets, m_DebugMode));
    m_Variables.emplace(name, std::make_pair(GetType<T>(), size));
    return varMap.at(size);
}

#define instantiate_specialization(T)                                          \
    template Variable<T> &ADIOS::DefineVariable<T>(                            \
        const std::string &, const Dims, const Dims, const Dims);
ADIOS_FOREACH_TYPE_1ARG(instantiate_specialization)
#undef instantiate_specialization

// -----------------------------------------------------------------------------
// template specializations of DefineVariable:
// -----------------------------------------------------------------------------

template <class T>
unsigned int ADIOS::GetVariableIndex(const std::string &name)
{
    auto itVariable = m_Variables.find(name);
    CheckVariableName(
        itVariable, name,
        "in call to GetVariable<" + GetType<T>() +
            ">, or call to GetVariableCompound if <T> = <compound>\n");
    return itVariable->second.second;
}

// Get template specialization
template <typename T>
Variable<T> &ADIOS::GetVariable(const std::string &name)
{
    return GetVarMap<T>().at(GetVariableIndex<T>(name));
}

#define instantiate_specialization(T)                                          \
    template Variable<T> &ADIOS::GetVariable<T>(const std::string &);
ADIOS_FOREACH_TYPE_1ARG(instantiate_specialization)
#undef instantiate_specialization

} // end namespace adios

#endif /* ADIOS_TCC_ */
