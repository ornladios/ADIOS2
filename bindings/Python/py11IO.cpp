/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * IO.cpp
 *
 *  Created on: Mar 14, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "py11IO.h"

#include "adios2/ADIOSMacros.h"
#include "adios2/helper/adiosFunctions.h" //GetType<T>

#include "py11types.h"

namespace adios2
{
namespace py11
{

IO::IO(core::IO &io, const bool debugMode) : m_IO(io), m_DebugMode(debugMode) {}

void IO::SetEngine(const std::string type) noexcept { m_IO.SetEngine(type); }

void IO::SetParameters(const Params &parameters) noexcept
{
    m_IO.SetParameters(parameters);
}

void IO::SetParameter(const std::string key, const std::string value) noexcept
{
    m_IO.SetParameter(key, value);
}

const Params &IO::GetParameters() const noexcept
{
    return m_IO.GetParameters();
}

unsigned int IO::AddTransport(const std::string type, const Params &parameters)
{
    return m_IO.AddTransport(type, parameters);
}

core::VariableBase *IO::DefineVariable(const std::string &name,
                                       std::string & /*stringValue*/)
{
    return &m_IO.DefineVariable<std::string>(name, Dims(), Dims(), Dims(),
                                             false);
}

core::VariableBase *IO::DefineVariable(const std::string &name,
                                       const Dims &shape, const Dims &start,
                                       const Dims &count,
                                       const bool isConstantDims,
                                       pybind11::array &array)
{
    core::VariableBase *variable = nullptr;

    if (false)
    {
    }
#define declare_type(T)                                                        \
    else if (pybind11::isinstance<                                             \
                 pybind11::array_t<T, pybind11::array::c_style>>(array))       \
    {                                                                          \
        variable = &m_IO.DefineVariable<T>(name, shape, start, count,          \
                                           isConstantDims);                    \
    }
    ADIOS2_FOREACH_NUMPY_TYPE_1ARG(declare_type)
#undef declare_type
    else
    {
        if (m_DebugMode)
        {
            throw std::invalid_argument(
                "ERROR: variable " + name +
                " can't be defined, either type is not "
                "supported or is not memory "
                "contiguous, in call to DefineVariable\n");
        }
    }

    return variable;
}

core::VariableBase *IO::InquireVariable(const std::string &name) noexcept
{
    const std::string type(m_IO.InquireVariableType(name));
    core::VariableBase *variable = nullptr;

    if (type == "unknown")
    {
    }
#define declare_template_instantiation(T)                                      \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        variable = m_IO.InquireVariable<T>(name);                              \
    }
    ADIOS2_FOREACH_PYTHON_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

    return variable;
}

core::AttributeBase *IO::DefineAttribute(const std::string &name,
                                         pybind11::array &array)
{
    core::AttributeBase *attribute = nullptr;

    if (false)
    {
    }
#define declare_type(T)                                                        \
    else if (pybind11::isinstance<                                             \
                 pybind11::array_t<T, pybind11::array::c_style>>(array))       \
    {                                                                          \
        attribute = &m_IO.DefineAttribute<T>(                                  \
            name, reinterpret_cast<const T *>(array.data()), array.size());    \
    }
    ADIOS2_FOREACH_NUMPY_ATTRIBUTE_TYPE_1ARG(declare_type)
#undef declare_type
    else
    {
        if (m_DebugMode)
        {
            throw std::invalid_argument(
                "ERROR: attribute " + name +
                " can't be defined, either type is not "
                "supported or is not memory "
                "contiguous, in call to DefineAttribute\n");
        }
    }

    return attribute;
}

core::AttributeBase *
IO::DefineAttribute(const std::string &name,
                    const std::vector<std::string> &strings)
{
    return &m_IO.DefineAttribute(name, strings.data(), strings.size());
}

core::AttributeBase *IO::InquireAttribute(const std::string &name) noexcept
{
    const std::string type(m_IO.InquireAttributeType(name));
    core::AttributeBase *attribute = nullptr;

    if (type == "unknown")
    {
    }
#define declare_template_instantiation(T)                                      \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        attribute = m_IO.InquireAttribute<T>(name);                            \
    }
    ADIOS2_FOREACH_ATTRIBUTE_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

    return attribute;
}

Engine IO::Open(const std::string &name, const int openMode)
{
    return Engine(m_IO, name, static_cast<adios2::Mode>(openMode),
                  m_IO.m_MPIComm);
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

std::string IO::EngineType() const noexcept { return m_IO.m_EngineType; }

} // end namespace py11
} // end namespace adios2
