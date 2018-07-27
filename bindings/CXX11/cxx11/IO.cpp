/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * IO.cpp :
 *
 *  Created on: Jun 4, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "IO.h"
#include "IO.tcc"

#include "adios2/ADIOSMPI.h"
#include "adios2/core/IO.h"

namespace adios2
{

bool IO::InConfigFile() const noexcept { return m_IO.InConfigFile(); }

void IO::SetEngine(const std::string engineType) noexcept
{
    m_IO.SetEngine(engineType);
}

void IO::SetParameter(const std::string key, const std::string value) noexcept
{
    m_IO.SetParameter(key, value);
}

void IO::SetParameters(const Params &parameters) noexcept
{
    m_IO.SetParameters(parameters);
}

const Params &IO::GetParameters() const noexcept { return m_IO.m_Parameters; }

unsigned int IO::AddTransport(const std::string type, const Params &parameters)
{
    return m_IO.AddTransport(type, parameters);
}

void IO::SetTransportParameter(const unsigned int transportIndex,
                               const std::string key, const std::string value)
{
    m_IO.SetTransportParameter(transportIndex, key, value);
}

bool IO::RemoveVariable(const std::string &name) noexcept
{
    return m_IO.RemoveVariable(name);
}

void IO::RemoveAllVariables() noexcept { m_IO.RemoveAllVariables(); }

bool IO::RemoveAttribute(const std::string &name) noexcept
{
    return m_IO.RemoveAttribute(name);
}

void IO::RemoveAllAttributes() noexcept { m_IO.RemoveAllAttributes(); }

Engine IO::Open(const std::string &name, const Mode mode, MPI_Comm comm)
{
    return Engine(&m_IO.Open(name, mode, comm));
}

Engine IO::Open(const std::string &name, const Mode mode)
{
    return Engine(&m_IO.Open(name, mode));
}

void IO::FlushAll() { m_IO.FlushAll(); }

std::map<std::string, Params> IO::AvailableVariables() noexcept
{
    return m_IO.GetAvailableVariables();
}

std::map<std::string, Params> IO::AvailableAttributes() noexcept
{
    return m_IO.GetAvailableAttributes();
}

size_t IO::AddOperation(const Operator op, const Params &parameters) noexcept
{
    return m_IO.AddOperation(*op.m_Operator, parameters);
}

std::string IO::EngineType() const noexcept { return m_IO.m_EngineType; }

// PRIVATE
IO::IO(core::IO &io) : m_IO(io) {}

// Explicit declaration of the public template methods
// Limits the types
#define declare_template_instantiation(T)                                      \
    template Variable<T> IO::DefineVariable(const std::string &, const Dims &, \
                                            const Dims &, const Dims &,        \
                                            const bool);                       \
                                                                               \
    template Variable<T> IO::InquireVariable<T>(const std::string &) noexcept;

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

#define declare_template_instantiation(T)                                      \
    template Attribute<T> IO::DefineAttribute(const std::string &, const T *,  \
                                              const size_t);                   \
                                                                               \
    template Attribute<T> IO::DefineAttribute(const std::string &, const T &); \
                                                                               \
    template Attribute<T> IO::InquireAttribute<T>(const std::string &) noexcept;
ADIOS2_FOREACH_ATTRIBUTE_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace adios2
