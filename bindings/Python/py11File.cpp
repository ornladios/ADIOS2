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
#include "py11File.tcc"

#include <algorithm>
#include <iostream>

#include "adios2/common/ADIOSMacros.h"
#include "adios2/helper/adiosCommDummy.h"
#include "adios2/helper/adiosFunctions.h"

#ifdef ADIOS2_HAVE_MPI
#include "adios2/helper/adiosCommMPI.h"
#endif

#include "py11types.h"

namespace adios2
{
namespace py11
{

#ifdef ADIOS2_HAVE_MPI
File::File(const std::string &name, const std::string mode, MPI_Comm comm,
           const std::string engineType)
: m_Name(name), m_Mode(mode),
  m_Stream(std::make_shared<core::Stream>(
      name, ToMode(mode), helper::CommFromMPI(comm), engineType, "Python"))
{
}

File::File(const std::string &name, const std::string mode, MPI_Comm comm,
           const std::string &configFile, const std::string ioInConfigFile)
: m_Name(name), m_Mode(mode),
  m_Stream(std::make_shared<core::Stream>(name, ToMode(mode),
                                          helper::CommFromMPI(comm), configFile,
                                          ioInConfigFile, "Python"))
{
}
#endif

File::File(const std::string &name, const std::string mode,
           const std::string engineType)
: m_Name(name), m_Mode(mode), m_Stream(std::make_shared<core::Stream>(
                                  name, ToMode(mode), engineType, "Python"))
{
}

File::File(const std::string &name, const std::string mode,
           const std::string &configFile, const std::string ioInConfigFile)
: m_Name(name), m_Mode(mode),
  m_Stream(std::make_shared<core::Stream>(name, ToMode(mode), configFile,
                                          ioInConfigFile, "Python"))
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
                        shape, start, count, vParams(), endStep);              \
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
                 const Dims &shape, const Dims &start, const Dims &count,
                 const adios2::vParams &operations, const bool endStep)
{
    if (false)
    {
    }
#define declare_type(T)                                                        \
    else if (pybind11::isinstance<                                             \
                 pybind11::array_t<T, pybind11::array::c_style>>(array))       \
    {                                                                          \
        m_Stream->Write(name, reinterpret_cast<const T *>(array.data()),       \
                        shape, start, count, operations, endStep);             \
    }
    ADIOS2_FOREACH_NUMPY_TYPE_1ARG(declare_type)
#undef declare_type
    else
    {
        throw std::invalid_argument(
            "ERROR: adios2 file write variable " + name +
            ", either numpy type is not supported or is not "
            "c_style memory contiguous, in call to write with operations\n");
    }
}

void File::Write(const std::string &name, const pybind11::array &array,
                 const bool isLocalValue, const bool endStep)
{
    if (isLocalValue)
    {
        Write(name, array, {adios2::LocalValueDim}, {}, {}, endStep);
    }
    else
    {
        Write(name, array, {}, {}, {}, endStep);
    }
}

void File::Write(const std::string &name, const std::string &stringValue,
                 const bool isLocalValue, const bool endStep)
{
    m_Stream->Write(name, stringValue, isLocalValue, endStep);
}

bool File::GetStep() const
{
    return const_cast<File *>(this)->m_Stream->GetStep();
}

std::vector<std::string> File::ReadString(const std::string &name,
                                          const size_t blockID)
{
    return m_Stream->Read<std::string>(name, blockID);
}

std::vector<std::string> File::ReadString(const std::string &name,
                                          const size_t stepStart,
                                          const size_t stepCount,
                                          const size_t blockID)
{
    return m_Stream->Read<std::string>(name, Box<size_t>(stepStart, stepCount),
                                       blockID);
}

pybind11::array File::Read(const std::string &name, const size_t blockID)
{
    return Read(name, {}, {}, blockID);
}

pybind11::array File::Read(const std::string &name, const Dims &start,
                           const Dims &count, const size_t blockID)
{
    const std::string type = m_Stream->m_IO->InquireVariableType(name);

    if (type == helper::GetType<std::string>())
    {
        const std::string value =
            m_Stream->Read<std::string>(name, blockID).front();
        pybind11::array_t<char> pyArray(Dims{value.size()});
        std::copy(value.begin(), value.end(), pyArray.mutable_data());
        return pyArray;
    }

    return Read(name, start, count, 0, 0, blockID);
}

pybind11::array File::Read(const std::string &name, const Dims &start,
                           const Dims &count, const size_t stepStart,
                           const size_t stepCount, const size_t blockID)
{
    const std::string type = m_Stream->m_IO->InquireVariableType(name);

    if (type.empty())
    {
    }
#define declare_type(T)                                                        \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        return DoRead<T>(name, start, count, stepStart, stepCount, blockID);   \
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
    const std::string type =
        m_Stream->m_IO->InquireAttributeType(name, variableName, separator);

    if (type.empty())
    {
    }
#define declare_type(T)                                                        \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        core::Attribute<T> *attribute = m_Stream->m_IO->InquireAttribute<T>(   \
            name, variableName, separator);                                    \
        if (attribute->m_IsSingleValue)                                        \
        {                                                                      \
            pybind11::array_t<T> pyArray({});                                  \
            pyArray.mutable_data()[0] = attribute->m_DataSingleValue;          \
        }                                                                      \
        pybind11::array_t<T> pyArray(attribute->m_Elements);                   \
        m_Stream->ReadAttribute<T>(name, pyArray.mutable_data(), variableName, \
                                   separator);                                 \
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

size_t File::CurrentStep() const { return m_Stream->CurrentStep(); }

size_t File::Steps() const { return m_Stream->Steps(); }

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
