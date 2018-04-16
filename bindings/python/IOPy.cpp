/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * IOPy.cpp
 *
 *  Created on: Mar 14, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "IOPy.h"
#include "typesPy.h"

#include "adios2/ADIOSMacros.h"
#include "adios2/helper/adiosFunctions.h" //GetType<T>

namespace adios2
{

IOPy::IOPy(IO &io, const bool debugMode) : m_IO(io), m_DebugMode(debugMode) {}

void IOPy::SetEngine(const std::string type) noexcept { m_IO.SetEngine(type); }

void IOPy::SetParameters(const Params &parameters) noexcept
{
    m_IO.SetParameters(parameters);
}

void IOPy::SetParameter(const std::string key, const std::string value) noexcept
{
    m_IO.SetParameter(key, value);
}

const Params &IOPy::GetParameters() const noexcept
{
    return m_IO.GetParameters();
}

unsigned int IOPy::AddTransport(const std::string type,
                                const Params &parameters)
{
    return m_IO.AddTransport(type, parameters);
}

VariableBase *IOPy::DefineVariable(const std::string &name,
                                   std::string &stringValue)
{
    return &m_IO.DefineVariable<std::string>(name, Dims(), Dims(), Dims(),
                                             false, &stringValue);
}

VariableBase *IOPy::DefineVariable(const std::string &name, const Dims &shape,
                                   const Dims &start, const Dims &count,
                                   const bool isConstantDims,
                                   pybind11::array &array)
{
    VariableBase *variable = nullptr;

    if (false)
    {
    }
#define declare_type(T)                                                        \
    else if (pybind11::isinstance<                                             \
                 pybind11::array_t<T, pybind11::array::c_style>>(array))       \
    {                                                                          \
        variable = &m_IO.DefineVariable<T>(                                    \
            name, shape, start, count, isConstantDims,                         \
            reinterpret_cast<T *>(const_cast<void *>(array.data())));          \
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

VariableBase *IOPy::InquireVariable(const std::string &name) noexcept
{
    const std::string type(m_IO.InquireVariableType(name));
    adios2::VariableBase *variable = nullptr;

    if (type == "unknown")
    {
    }
#define declare_template_instantiation(T)                                      \
    else if (type == adios2::GetType<T>())                                     \
    {                                                                          \
        variable = m_IO.InquireVariable<T>(name);                              \
    }
    ADIOS2_FOREACH_NUMPY_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

    return variable;
}

AttributeBase *IOPy::DefineAttribute(const std::string &name,
                                     pybind11::array &array)
{
    AttributeBase *attribute = nullptr;

    if (false)
    {
    }
#define declare_type(T)                                                        \
    else if (pybind11::isinstance<                                             \
                 pybind11::array_t<T, pybind11::array::c_style>>(array))       \
    {                                                                          \
        attribute = &m_IO.DefineAttribute<T>(                                  \
            name, reinterpret_cast<T *>(const_cast<void *>(array.data())),     \
            array.size());                                                     \
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

AttributeBase *IOPy::DefineAttribute(const std::string &name,
                                     const std::vector<std::string> &strings)
{
    return &m_IO.DefineAttribute(name, strings.data(), strings.size());
}

EnginePy IOPy::Open(const std::string &name, const int openMode)
{
    return EnginePy(m_IO, name, static_cast<adios2::Mode>(openMode),
                    m_IO.m_MPIComm);
}

void IOPy::FlushAll() { m_IO.FlushAll(); }

} // end namespace adios2
