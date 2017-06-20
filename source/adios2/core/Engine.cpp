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

// should these functions throw an exception?

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
void Engine::Init() {}

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
        DoWrite(m_IO.GetVariableCompound(variableName), values);
    }
#define declare_type(T)                                                        \
    else if (type == GetType<T>())                                             \
    {                                                                          \
        DoWrite(m_IO.GetVariable<T>(variableName),                             \
                reinterpret_cast<const T *>(values));                          \
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
Variable<char> *Engine::InquireVariableChar(const std::string &name,
                                            const bool readIn)
{
    return nullptr;
}
Variable<unsigned char> *Engine::InquireVariableUChar(const std::string &name,
                                                      const bool readIn)
{
    return nullptr;
}
Variable<short> *Engine::InquireVariableShort(const std::string &name,
                                              const bool readIn)
{
    return nullptr;
}
Variable<unsigned short> *Engine::InquireVariableUShort(const std::string &name,
                                                        const bool readIn)
{
    return nullptr;
}
Variable<int> *Engine::InquireVariableInt(const std::string &name,
                                          const bool readIn)
{
    return nullptr;
}
Variable<unsigned int> *Engine::InquireVariableUInt(const std::string &name,
                                                    const bool readIn)
{
    return nullptr;
}
Variable<long int> *Engine::InquireVariableLInt(const std::string &name,
                                                const bool readIn)
{
    return nullptr;
}
Variable<unsigned long int> *
Engine::InquireVariableULInt(const std::string &name, const bool readIn)
{
    return nullptr;
}
Variable<long long int> *Engine::InquireVariableLLInt(const std::string &name,
                                                      const bool readIn)
{
    return nullptr;
}
Variable<unsigned long long int> *
Engine::InquireVariableULLInt(const std::string &name, const bool readIn)
{
    return nullptr;
}

Variable<float> *Engine::InquireVariableFloat(const std::string &name,
                                              const bool readIn)
{
    return nullptr;
}
Variable<double> *Engine::InquireVariableDouble(const std::string &name,
                                                const bool readIn)
{
    return nullptr;
}
Variable<long double> *Engine::InquireVariableLDouble(const std::string &name,
                                                      const bool readIn)
{
    return nullptr;
}
Variable<cfloat> *Engine::InquireVariableCFloat(const std::string &name,
                                                const bool readIn)
{
    return nullptr;
}
Variable<cdouble> *Engine::InquireVariableCDouble(const std::string &name,
                                                  const bool readIn)
{
    return nullptr;
}

Variable<cldouble> *Engine::InquireVariableCLDouble(const std::string &name,
                                                    const bool readIn)
{
    return nullptr;
}

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

} // end namespace adios
