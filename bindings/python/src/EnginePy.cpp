/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * EnginePy.cpp
 *
 *  Created on: Mar 15, 2017
 *      Author: wgodoy
 */

#include <string>

#include "adios2/EnginePy.h"

#include "adios2/adiosPyFunctions.h"

namespace adios
{

EnginePy::EnginePy(ADIOSPy &adiosPy) : m_ADIOSPy{adiosPy} {}

EnginePy::~EnginePy() {}

void EnginePy::WritePy(VariablePy &variable, const pyArray &array)
{

    if (variable.m_IsVariableDefined == false) // here define variable
    {
        if (IsType<char>(array))
            DefineVariableInADIOS<char>(variable);
        else if (IsType<unsigned char>(array))
            DefineVariableInADIOS<unsigned char>(variable);
        else if (IsType<short>(array))
            DefineVariableInADIOS<short>(variable);
        else if (IsType<unsigned short>(array))
            DefineVariableInADIOS<unsigned short>(variable);
        else if (IsType<int>(array))
            DefineVariableInADIOS<int>(variable);
        else if (IsType<unsigned int>(array))
            DefineVariableInADIOS<unsigned int>(variable);
        else if (IsType<long int>(array))
            DefineVariableInADIOS<long int>(variable);
        else if (IsType<unsigned long int>(array))
            DefineVariableInADIOS<unsigned long int>(variable);
        else if (IsType<long long int>(array))
            DefineVariableInADIOS<long long int>(variable);
        else if (IsType<unsigned long long int>(array))
            DefineVariableInADIOS<unsigned long long int>(variable);
        else if (IsType<float>(array))
            DefineVariableInADIOS<float>(variable);
        else if (IsType<double>(array))
            DefineVariableInADIOS<double>(variable);
        else if (IsType<long double>(array))
            DefineVariableInADIOS<long double>(variable);
        else if (IsType<std::complex<float>>(array))
            DefineVariableInADIOS<std::complex<float>>(variable);
        else if (IsType<std::complex<double>>(array))
            DefineVariableInADIOS<std::complex<double>>(variable);
        else if (IsType<std::complex<long double>>(array))
            DefineVariableInADIOS<std::complex<long double>>(variable);
    }

    if (IsType<char>(array))
        WriteVariableInADIOS<char>(variable, array);
    else if (IsType<unsigned char>(array))
        WriteVariableInADIOS<unsigned char>(variable, array);
    else if (IsType<short>(array))
        WriteVariableInADIOS<short>(variable, array);
    else if (IsType<unsigned short>(array))
        WriteVariableInADIOS<unsigned short>(variable, array);
    else if (IsType<int>(array))
        WriteVariableInADIOS<int>(variable, array);
    else if (IsType<unsigned int>(array))
        WriteVariableInADIOS<unsigned int>(variable, array);
    else if (IsType<long int>(array))
        WriteVariableInADIOS<long int>(variable, array);
    else if (IsType<unsigned long int>(array))
        WriteVariableInADIOS<unsigned long int>(variable, array);
    else if (IsType<long long int>(array))
        WriteVariableInADIOS<long long int>(variable, array);
    else if (IsType<unsigned long long int>(array))
        WriteVariableInADIOS<unsigned long long int>(variable, array);
    else if (IsType<float>(array))
        WriteVariableInADIOS<float>(variable, array);
    else if (IsType<double>(array))
        WriteVariableInADIOS<double>(variable, array);
    else if (IsType<long double>(array))
        WriteVariableInADIOS<long double>(variable, array);
    else if (IsType<std::complex<float>>(array))
        WriteVariableInADIOS<std::complex<float>>(variable, array);
    else if (IsType<std::complex<double>>(array))
        WriteVariableInADIOS<std::complex<double>>(variable, array);
    else if (IsType<std::complex<long double>>(array))
        WriteVariableInADIOS<std::complex<long double>>(variable, array);
}

void EnginePy::Advance() { m_Engine->Advance(); }

void EnginePy::Close() { m_Engine->Close(-1); }

void EnginePy::GetEngineType() const
{
    std::cout << "Engine type " << m_Engine->m_EngineType << "\n";
}

} // end namespace
