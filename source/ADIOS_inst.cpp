/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS.tcc
 *   This contains the template specializatios for the ADIOS class
 */

#include "ADIOS.tcc"
#include "ADIOSMacros.h"

namespace adios
{

// -----------------------------------------------------------------------------
// template specializations of GetVarMap helper function
// -----------------------------------------------------------------------------

template <>
std::map<unsigned int, Variable<char>> &ADIOS::GetVariableMap()
{
    return m_Char;
}

template <>
std::map<unsigned int, Variable<unsigned char>> &ADIOS::GetVariableMap()
{
    return m_UChar;
}

template <>
std::map<unsigned int, Variable<short>> &ADIOS::GetVariableMap()
{
    return m_Short;
}

template <>
std::map<unsigned int, Variable<unsigned short>> &ADIOS::GetVariableMap()
{
    return m_UShort;
}

template <>
std::map<unsigned int, Variable<int>> &ADIOS::GetVariableMap()
{
    return m_Int;
}

template <>
std::map<unsigned int, Variable<unsigned int>> &ADIOS::GetVariableMap()
{
    return m_UInt;
}

template <>
std::map<unsigned int, Variable<long int>> &ADIOS::GetVariableMap()
{
    return m_LInt;
}

template <>
std::map<unsigned int, Variable<unsigned long int>> &ADIOS::GetVariableMap()
{
    return m_ULInt;
}

template <>
std::map<unsigned int, Variable<long long int>> &ADIOS::GetVariableMap()
{
    return m_LLInt;
}

template <>
std::map<unsigned int, Variable<unsigned long long int>> &
ADIOS::GetVariableMap()
{
    return m_ULLInt;
}

template <>
std::map<unsigned int, Variable<float>> &ADIOS::GetVariableMap()
{
    return m_Float;
}

template <>
std::map<unsigned int, Variable<double>> &ADIOS::GetVariableMap()
{
    return m_Double;
}

template <>
std::map<unsigned int, Variable<long double>> &ADIOS::GetVariableMap()
{
    return m_LDouble;
}

template <>
std::map<unsigned int, Variable<std::complex<float>>> &ADIOS::GetVariableMap()
{
    return m_CFloat;
}

template <>
std::map<unsigned int, Variable<std::complex<double>>> &ADIOS::GetVariableMap()
{
    return m_CDouble;
}

template <>
std::map<unsigned int, Variable<std::complex<long double>>> &
ADIOS::GetVariableMap()
{
    return m_CLDouble;
}

// -----------------------------------------------------------------------------
// explicit template instantiations of DefineVariable:
// -----------------------------------------------------------------------------

template <typename T>
Variable<T> &
ADIOS::DefineVariable(const std::string &name, const Dims globalDimensions,
                      const Dims localDimensions, const Dims offsets)
{
    auto &variableMap = GetVariableMap<T>();
    CheckVariableInput(name, globalDimensions);
    const unsigned int size = variableMap.size();
    variableMap.emplace(size,
                        Variable<T>(name, globalDimensions, localDimensions,
                                    offsets, m_DebugMode));
    m_Variables.emplace(name, std::make_pair(GetType<T>(), size));
    return variableMap.at(size);
}

#define define_template_instantiation(T)                                       \
    template Variable<T> &ADIOS::DefineVariable<T>(                            \
        const std::string &, const Dims, const Dims, const Dims);
ADIOS_FOREACH_TYPE_1ARG(define_template_instantiation)
#undef define_template_instatiation

// -----------------------------------------------------------------------------
// template specializations of GetVariable:
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

template <typename T>
Variable<T> &ADIOS::GetVariable(const std::string &name)
{
    return GetVariableMap<T>().at(GetVariableIndex<T>(name));
}

#define define_template_instatiation(T)                                        \
    template unsigned int ADIOS::GetVariableIndex<T>(const std::string &);     \
    template Variable<T> &ADIOS::GetVariable<T>(const std::string &);
ADIOS_FOREACH_TYPE_1ARG(define_template_instatiation)
template unsigned int ADIOS::GetVariableIndex<void>(const std::string &);
#undef define_template_instatiation

} // end namespace adios
