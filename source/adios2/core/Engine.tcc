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

template <class T>
void Engine::Write(Variable<T> &variable, const T *values)
{
    if (m_DebugMode)
    {
        variable.CheckDimsBeforeWrite("Write " + variable.m_Name);
    }

    DoWrite(variable, values);
}

template <class T>
void Engine::Write(Variable<T> &variable, const T values)
{
    const T val = values; // need an address for memory copy
    Write(variable, &values);
}

template <class T>
void Engine::Write(const std::string &variableName, const T *values)
{
    Write(m_IO.GetVariable<T>(variableName), values);
}

template <class T>
void Engine::Write(const std::string &variableName, const T values)
{
    const T val = values; // need an address for memory copy
    Write(m_IO.GetVariable<T>(variableName), &values);
}

} // end namespace adios

#endif /** ADIOS2_CORE_ENGINE_TCC_ */
