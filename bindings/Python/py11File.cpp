/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * py11File.cpp
 *
 *  Created on: Mar 6, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "py11File.h"

#include <algorithm>
#include <iostream>

#include "adios2/ADIOSMPI.h"
#include "adios2/ADIOSMacros.h"
#include "adios2/helper/adiosFunctions.h"

#include "py11types.h"

namespace adios2
{
namespace py11
{

File::File(const std::string &name, const std::string mode, MPI_Comm comm,
           const std::string engineType)
: m_Name(name), m_Mode(mode),
  m_Stream(std::make_shared<core::Stream>(name, ToMode(mode), comm, engineType,
                                          "Python"))
{
}

File::File(const std::string &name, const std::string mode, MPI_Comm comm,
           const std::string &configFile, const std::string ioInConfigFile)
: m_Name(name), m_Mode(mode),
  m_Stream(std::make_shared<core::Stream>(name, ToMode(mode), comm, configFile,
                                          ioInConfigFile, "Python"))
{
}

File::File(const std::string &name, const std::string mode,
           const std::string engineType)
: File(name, mode, MPI_COMM_SELF, engineType)
{
}

File::File(const std::string &name, const std::string mode,
           const std::string &configFile, const std::string ioInConfigFile)
: File(name, mode, MPI_COMM_SELF, configFile, ioInConfigFile)
{
}

void File::SetParameter(const std::string key, const std::string value) noexcept
{
    m_Stream->m_IO->SetParameter(key, value);
}

void File::SetParameters(const Params &parameters) noexcept
{
    m_Stream->m_IO->SetParameters(parameters);
}

size_t File::AddTransport(const std::string type, const Params &parameters)
{
    return m_Stream->m_IO->AddTransport(type, parameters);
}

std::map<std::string, adios2::Params> File::AvailableVariables() noexcept
{
    return m_Stream->m_IO->GetAvailableVariables();
}

std::map<std::string, adios2::Params> File::AvailableAttributes() noexcept
{
    return m_Stream->m_IO->GetAvailableAttributes();
}

void File::WriteAttribute(const std::string &name, const pybind11::array &array,
                          const std::string &variableName,
                          const std::string separator, const bool endStep)
{
    if (false)
    {
    }
#define declare_type(T)                                                        \
    else if (pybind11::isinstance<                                             \
                 pybind11::array_t<T, pybind11::array::c_style>>(array))       \
    {                                                                          \
        m_Stream->WriteAttribute(name,                                         \
                                 reinterpret_cast<const T *>(array.data()),    \
                                 static_cast<size_t>(array.size()),            \
                                 variableName, separator, endStep);            \
    }
    ADIOS2_FOREACH_NUMPY_ATTRIBUTE_TYPE_1ARG(declare_type)
#undef declare_type
    else
    {
        throw std::invalid_argument(
            "ERROR: adios2 file write attribute " + name +
            ", either numpy type is not supported or is not "
            "c_style memory contiguous, in call to write\n");
    }
}

void File::WriteAttribute(const std::string &name,
                          const std::string &stringValue,
                          const std::string &variableName,
                          const std::string separator, const bool endStep)
{
    m_Stream->WriteAttribute(name, stringValue, variableName, separator,
                             endStep);
}

void File::WriteAttribute(const std::string &name,
                          const std::vector<std::string> &stringArray,
                          const std::string &variableName,
                          const std::string separator, const bool endStep)
{
    m_Stream->WriteAttribute(name, stringArray.data(), stringArray.size(),
                             variableName, separator, endStep);
}

void File::Write(const std::string &name, const pybind11::array &array,
                 const Dims &shape, const Dims &start, const Dims &count,
                 const bool endStep)
{
    if (false)
    {
    }
#define declare_type(T)                                                        \
    else if (pybind11::isinstance<                                             \
                 pybind11::array_t<T, pybind11::array::c_style>>(array))       \
    {                                                                          \
        m_Stream->Write(name, reinterpret_cast<const T *>(array.data()),       \
                        shape, start, count, endStep);                         \
    }
    ADIOS2_FOREACH_NUMPY_TYPE_1ARG(declare_type)
#undef declare_type
    else
    {
        throw std::invalid_argument(
            "ERROR: adios2 file write variable " + name +
            ", either numpy type is not supported or is not "
            "c_style memory contiguous, in call to write\n");
    }
}

void File::Write(const std::string &name, const pybind11::array &array,
                 const bool endStep)
{
    Write(name, array, {}, {}, {}, endStep);
}

void File::Write(const std::string &name, const std::string &stringValue,
                 const bool endStep)
{
    m_Stream->Write(name, stringValue, endStep);
}

bool File::GetStep() const
{
    return const_cast<File *>(this)->m_Stream->GetStep();
}

std::vector<std::string> File::ReadString(const std::string &name)
{
    return m_Stream->Read<std::string>(name);
}

std::vector<std::string> File::ReadString(const std::string &name,
                                          const size_t stepStart,
                                          const size_t stepCount)
{
    return m_Stream->Read<std::string>(name, Box<size_t>(stepStart, stepCount));
}

pybind11::array File::Read(const std::string &name)
{
    const DataType type = m_Stream->m_IO->InquireVariableType(name);

    if (type == DataType::String)
    {
        const std::string value = m_Stream->Read<std::string>(name).front();
        pybind11::array pyArray(pybind11::dtype::of<char>(),
                                Dims{value.size()});
        char *pyPtr =
            reinterpret_cast<char *>(const_cast<void *>(pyArray.data()));
        std::copy(value.begin(), value.end(), pyPtr);
        return pyArray;
    }
#define declare_type(T)                                                        \
    else if (type == helper::GetDataType<T>())                                 \
    {                                                                          \
        core::Variable<T> &variable =                                          \
            *m_Stream->m_IO->InquireVariable<T>(name);                         \
        Dims pyCount;                                                          \
        if (variable.m_SingleValue)                                            \
        {                                                                      \
            pyCount = {1};                                                     \
            pybind11::array pyArray(pybind11::dtype::of<T>(), pyCount);        \
            m_Stream->Read<T>(name, reinterpret_cast<T *>(                     \
                                        const_cast<void *>(pyArray.data())));  \
            return pyArray;                                                    \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            const Dims zerosStart(variable.m_Shape.size(), 0);                 \
            return Read(name, zerosStart, variable.m_Shape);                   \
        }                                                                      \
    }
    ADIOS2_FOREACH_NUMPY_TYPE_1ARG(declare_type)
#undef declare_type
    else
    {
        throw std::invalid_argument(
            "ERROR: adios2 file read variable " + name +
            ", type can't be mapped to a numpy type, in call to read\n");
    }
    return pybind11::array();
}

pybind11::array File::Read(const std::string &name, const Dims &selectionStart,
                           const Dims &selectionCount)
{
    const DataType type = m_Stream->m_IO->InquireVariableType(name);

    if (type.empty())
    {
    }
#define declare_type(T)                                                        \
    else if (type == helper::GetDataType<T>())                                 \
    {                                                                          \
        pybind11::array pyArray(pybind11::dtype::of<T>(), selectionCount);     \
        m_Stream->Read<T>(                                                     \
            name, reinterpret_cast<T *>(const_cast<void *>(pyArray.data())),   \
            Box<Dims>(selectionStart, selectionCount));                        \
        return pyArray;                                                        \
    }
    ADIOS2_FOREACH_NUMPY_TYPE_1ARG(declare_type)
#undef declare_type

    throw std::invalid_argument(
        "ERROR: adios2 file read variable " + name +
        ", type can't be mapped to a numpy type, in call to read\n");
}

pybind11::array File::Read(const std::string &name, const Dims &selectionStart,
                           const Dims &selectionCount,
                           const size_t stepSelectionStart,
                           const size_t stepSelectionCount)
{
    // shape of the returned numpy array
    Dims shapePy(selectionCount.size() + 1);
    shapePy[0] = stepSelectionCount;
    for (auto i = 1; i < shapePy.size(); ++i)
    {
        shapePy[i] = selectionCount[i - 1];
    }

    const DataType type = m_Stream->m_IO->InquireVariableType(name);

    if (type.empty())
    {
    }
#define declare_type(T)                                                        \
    else if (type == helper::GetDataType<T>())                                 \
    {                                                                          \
        pybind11::array pyArray(pybind11::dtype::of<T>(), shapePy);            \
        m_Stream->Read<T>(                                                     \
            name, reinterpret_cast<T *>(const_cast<void *>(pyArray.data())),   \
            Box<Dims>(selectionStart, selectionCount),                         \
            Box<size_t>(stepSelectionStart, stepSelectionCount));              \
        return pyArray;                                                        \
    }
    ADIOS2_FOREACH_NUMPY_TYPE_1ARG(declare_type)
#undef declare_type
    else
    {
        throw std::invalid_argument(
            "ERROR: adios2 file read variable " + name +
            ", type can't be mapped to a numpy type, in call to read\n");
    }
    return pybind11::array();
}

pybind11::array File::ReadAttribute(const std::string &name,
                                    const std::string &variableName,
                                    const std::string separator)
{
    const DataType type =
        m_Stream->m_IO->InquireAttributeType(name, variableName, separator);

    if (type.empty())
    {
    }
#define declare_type(T)                                                        \
    else if (type == helper::GetDataType<T>())                                 \
    {                                                                          \
        core::Attribute<T> *attribute = m_Stream->m_IO->InquireAttribute<T>(   \
            name, variableName, separator);                                    \
        pybind11::array pyArray(pybind11::dtype::of<T>(),                      \
                                attribute->m_Elements);                        \
        m_Stream->ReadAttribute<T>(                                            \
            name, reinterpret_cast<T *>(const_cast<void *>(pyArray.data())),   \
            variableName, separator);                                          \
        return pyArray;                                                        \
    }
    ADIOS2_FOREACH_NUMPY_ATTRIBUTE_TYPE_1ARG(declare_type)
#undef declare_type
    else
    {
        throw std::invalid_argument(
            "ERROR: adios2 file read attribute " + name +
            ", type can't be mapped to a numpy type, in call to read\n");
    }
    return pybind11::array();
}

std::vector<std::string>
File::ReadAttributeString(const std::string &name,
                          const std::string &variableName,
                          const std::string separator)
{
    const core::Attribute<std::string> *attribute =
        m_Stream->m_IO->InquireAttribute<std::string>(name, variableName,
                                                      separator);

    if (attribute == nullptr)
    {
        return std::vector<std::string>();
    }

    std::vector<std::string> data(attribute->m_Elements);
    m_Stream->ReadAttribute<std::string>(name, data.data(), variableName,
                                         separator);
    return data;
}

void File::EndStep() { m_Stream->EndStep(); }

void File::Close()
{
    m_Stream->Close();
    m_Stream.reset();
}

size_t File::CurrentStep() const { return m_Stream->CurrentStep(); };

// PRIVATE
adios2::Mode File::ToMode(const std::string mode) const
{
    adios2::Mode modeCpp = adios2::Mode::Undefined;
    if (mode == "w")
    {
        modeCpp = adios2::Mode::Write;
    }
    else if (mode == "a")
    {
        modeCpp = adios2::Mode::Append;
    }
    else if (mode == "r")
    {
        modeCpp = adios2::Mode::Read;
    }
    else
    {
        throw std::invalid_argument(
            "ERROR: adios2 mode " + mode + " for file " + m_Name +
            " not supported, only \"r\", \"w\" and \"a\" (read, write, append) "
            "are valid modes, in call to open\n");
    }

    return modeCpp;
}

} // end namespace py11
} // end namespace adios2
