/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Engine.cpp
 *
 *  Created on: Dec 19, 2016
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "Engine.h"
#include "Engine.tcc"

/// \cond EXCLUDE_FROM_DOXYGEN
#include <ios> //std::ios_base::failure
#include <set>
/// \endcond

#include "adios2/helper/adiosFunctions.h" //GetType<T>

#include <iostream>

namespace adios2
{

Engine::Engine(const std::string engineType, IO &io, const std::string &name,
               const OpenMode openMode, MPI_Comm mpiComm)
: m_EngineType(engineType), m_IO(io), m_Name(name), m_OpenMode(openMode),
  m_MPIComm(mpiComm), m_DebugMode(io.m_DebugMode)
{
}

void Engine::SetCallBack(
    std::function<void(const void *, std::string, std::string, std::string,
                       std::vector<size_t>)>
        callback)
{
}

void Engine::Write(VariableBase &variable, const void *values)
{
    DoWrite(variable.m_Name, values);
}

void Engine::Write(const std::string &variableName, const void *values)
{
    DoWrite(variableName, values);
}

void Engine::Advance(const float /*timeout_sec*/) {}
void Engine::Advance(const AdvanceMode /*mode*/, const float /*timeout_sec*/) {}
void Engine::AdvanceAsync(
    AdvanceMode /*mode*/,
    std::function<void(std::shared_ptr<adios2::Engine>)> /*callback*/)
{
}

AdvanceStatus Engine::GetAdvanceStatus() { return m_AdvanceStatus; }

void Engine::Close(const int /*transportIndex*/) {}

// READ
void Engine::Release() {}
void Engine::PerformReads(ReadMode /*mode*/){};

// PROTECTED
void Engine::Init() { std::cout << "Inside Engine::Init()" << std::endl; }

void Engine::InitParameters() {}

void Engine::InitTransports() {}

// DoWrite
#define declare_type(T)                                                        \
    void Engine::DoWrite(Variable<T> &variable, const T *values)               \
    {                                                                          \
        ThrowUp("Write");                                                      \
    }
ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

void Engine::DoWrite(VariableCompound &variable, const void *values)
{ // TODO
}

void Engine::DoWrite(const std::string &variableName, const void *values)
{
    const std::string type(m_IO.GetVariableType(variableName));
    if (m_DebugMode)
    {
        if (type.empty())
        {
            throw std::invalid_argument(
                "ERROR: variable " + variableName +
                " was not created with IO.DefineVariable for Engine " + m_Name +
                ", in call to Write\n");
        }
    }

    if (type == "compound")
    {
        VariableCompound &variable = m_IO.GetVariableCompound(variableName);

        if (m_DebugMode)
        {
            variable.CheckDimsBeforeWrite("Write " + variable.m_Name);
        }

        DoWrite(variable, values);
    }
#define declare_type(T)                                                        \
    else if (type == GetType<T>())                                             \
    {                                                                          \
        Variable<T> &variable = m_IO.GetVariable<T>(variableName);             \
                                                                               \
        if (m_DebugMode)                                                       \
        {                                                                      \
            variable.CheckDimsBeforeWrite("Write " + variable.m_Name);         \
        }                                                                      \
                                                                               \
        DoWrite(variable, reinterpret_cast<const T *>(values));                \
    }
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type
} // end DoWrite

// READ
VariableBase *Engine::InquireVariableUnknown(const std::string &name,
                                             const bool readIn)
{
    return nullptr;
}

#define define(T, L)                                                           \
    template <>                                                                \
    Variable<T> *Engine::InquireVariable<T>(const std::string &variableName,   \
                                            const bool readIn)                 \
    {                                                                          \
        return InquireVariable##L(variableName, readIn);                       \
    }                                                                          \
    Variable<T> *Engine::InquireVariable##L(const std::string &name,           \
                                            const bool readIn)                 \
    {                                                                          \
        return nullptr;                                                        \
    }
ADIOS2_FOREACH_TYPE_2ARGS(define)
#undef define

#define declare_type(T)                                                        \
    void Engine::DoScheduleRead(Variable<T> &variable, const T *values)        \
    {                                                                          \
        ThrowUp("ScheduleRead");                                               \
    }                                                                          \
                                                                               \
    void Engine::DoScheduleRead(const std::string &variableName,               \
                                const T *values)                               \
    {                                                                          \
        ThrowUp("ScheduleRead");                                               \
    }
ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

// PRIVATE
void Engine::ThrowUp(const std::string function) const
{
    throw std::invalid_argument("ERROR: Engine derived class " + m_EngineType +
                                " doesn't implement function " + function +
                                "\n");
}

#define declare_template_instantiation(T)                                      \
    template void Engine::Write<T>(Variable<T> &, const T *);                  \
    template void Engine::Write<T>(Variable<T> &, const T);                    \
                                                                               \
    template void Engine::Write<T>(const std::string &, const T *);            \
    template void Engine::Write<T>(const std::string &, const T);

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace adios
