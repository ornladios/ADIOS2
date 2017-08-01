/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Engine.inl
 *
 *  Created on: May 17, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_CORE_ENGINE_INL_
#define ADIOS2_CORE_ENGINE_INL_
#ifndef ADIOS2_CORE_ENGINE_H_
#error "Inline file should only be included from it's header, never on it's own"
#endif

namespace adios2
{

template <class T>
T *Engine::AllocateVariable(Variable<T> &variable, T fillValue)
{
    throw std::invalid_argument("ERROR: type not supported for variable " +
                                variable.m_Name + " in call to \n");
}

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
void Engine::Write(const std::string &variableName, const T *values)
{
    Write(m_IO.GetVariable<T>(variableName), values);
}

template <class T>
void Engine::Write(Variable<T> &variable, const T values)
{
    const T val = values; // need an address for memory copy
    Write(variable, &values);
}

template <class T>
void Engine::Write(const std::string &variableName, const T values)
{
    const T val = values; // need an address for memory copy
    Write(m_IO.GetVariable<T>(variableName), &values);
}

template <class T>
void Engine::Read(Variable<T> &variable, T *values)
{
    DoScheduleRead(variable, values);
    PerformReads(ReadMode::Blocking);
}

template <class T>
void Engine::Read(const std::string &variableName, T *values)
{
    DoScheduleRead(variableName, values);
    PerformReads(ReadMode::Blocking);
}

template <class T>
void Engine::Read(Variable<T> &variable, T &values)
{
    DoScheduleRead(variable, &values);
    PerformReads(ReadMode::Blocking);
}

template <class T>
void Engine::Read(const std::string &variableName, T &values)
{
    DoScheduleRead(variableName, &values);
    PerformReads(ReadMode::Blocking);
}

template <class T>
void Engine::Read(Variable<T> &variable)
{
    DoScheduleRead(variable);
    PerformReads(ReadMode::Blocking);
}

template <class T>
void Engine::Read(const std::string &variableName)
{
    // TODO
    // DoScheduleRead(variableName);
    // PerformReads(PerformReadMode::BLOCKINGREAD);
}

template <class T>
void Engine::ScheduleRead(Variable<T> &variable, T *values)
{
    DoScheduleRead(variable, values);
}

template <class T>
void Engine::ScheduleRead(const std::string &variableName, T *values)
{
    DoScheduleRead(variableName, values);
}

template <class T>
void Engine::ScheduleRead(Variable<T> &variable, T &values)
{
    DoScheduleRead(variable, &values);
}

template <class T>
void Engine::ScheduleRead(const std::string &variableName, T &values)
{
    DoScheduleRead(variableName, &values);
}

} // end namespace adios

#endif /* ADIOS2_CORE_ENGINE_INL_ */
