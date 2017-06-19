/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Engine.tcc
 *
 *  Created on: Jun 2, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_CORE_ENGINE_TCC_
#define ADIOS2_CORE_ENGINE_TCC_

#include "Engine.h"

namespace adios2
{

template <>
Variable<char> *Engine::InquireVariable<char>(const std::string &variableName,
                                              const bool readIn)
{
    return InquireVariableChar(variableName, readIn);
}

template <>
Variable<unsigned char> *
Engine::InquireVariable<unsigned char>(const std::string &variableName,
                                       const bool readIn)
{
    return InquireVariableUChar(variableName, readIn);
}

template <>
Variable<short> *Engine::InquireVariable<short>(const std::string &variableName,
                                                const bool readIn)
{
    return InquireVariableShort(variableName, readIn);
}

template <>
Variable<unsigned short> *
Engine::InquireVariable<unsigned short>(const std::string &variableName,
                                        const bool readIn)
{
    return InquireVariableUShort(variableName, readIn);
}

template <>
Variable<int> *Engine::InquireVariable<int>(const std::string &variableName,
                                            const bool readIn)
{
    return InquireVariableInt(variableName, readIn);
}

template <>
Variable<unsigned int> *
Engine::InquireVariable<unsigned int>(const std::string &variableName,
                                      const bool readIn)
{
    return InquireVariableUInt(variableName, readIn);
}

template <>
Variable<long int> *
Engine::InquireVariable<long int>(const std::string &variableName,
                                  const bool readIn)
{
    return InquireVariableLInt(variableName, readIn);
}

template <>
Variable<long long int> *
Engine::InquireVariable<long long int>(const std::string &variableName,
                                       const bool readIn)
{
    return InquireVariableLLInt(variableName, readIn);
}

template <>
Variable<unsigned long int> *
Engine::InquireVariable<unsigned long int>(const std::string &variableName,
                                           const bool readIn)
{
    return InquireVariableULInt(variableName, readIn);
}

template <>
Variable<unsigned long long int> *
Engine::InquireVariable<unsigned long long int>(const std::string &variableName,
                                                const bool readIn)
{
    return InquireVariableULLInt(variableName, readIn);
}

template <>
Variable<float> *Engine::InquireVariable<float>(const std::string &variableName,
                                                const bool readIn)
{
    return InquireVariableFloat(variableName, readIn);
}

template <>
Variable<double> *
Engine::InquireVariable<double>(const std::string &variableName,
                                const bool readIn)
{
    return InquireVariableDouble(variableName, readIn);
}

template <>
Variable<long double> *
Engine::InquireVariable<long double>(const std::string &variableName,
                                     const bool readIn)
{
    return InquireVariableLDouble(variableName, readIn);
}

template <>
Variable<cfloat> *
Engine::InquireVariable<cfloat>(const std::string &variableName,
                                const bool readIn)
{
    return InquireVariableCFloat(variableName, readIn);
}

template <>
Variable<cdouble> *
Engine::InquireVariable<cdouble>(const std::string &variableName,
                                 const bool readIn)
{
    return InquireVariableCDouble(variableName, readIn);
}

template <>
Variable<cldouble> *
Engine::InquireVariable<cldouble>(const std::string &variableName,
                                  const bool readIn)
{
    return InquireVariableCLDouble(variableName, readIn);
}

} // end namespace adios

#endif /** ADIOS2_CORE_ENGINE_TCC_ */
