/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FilePy.cpp
 *
 *  Created on: Mar 6, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "FilePy.h"
#include "typesPy.h"

#include <algorithm>
#include <iostream>

#include "adios2/ADIOSMPI.h"
#include "adios2/ADIOSMacros.h"
#include "adios2/helper/adiosFunctions.h"

namespace adios2
{

FilePy::FilePy(const std::string &name, const std::string mode, MPI_Comm comm,
               const std::string engineType, const Params &parameters,
               const vParams &transportParameters)
: m_Name(name), m_Mode(mode)
{
    if (mode == "r")
    {
        m_Stream =
            std::make_shared<Stream>(name, adios2::Mode::Read, comm, engineType,
                                     parameters, transportParameters);
    }
    else if (mode == "w")
    {
        m_Stream = std::make_shared<Stream>(name, adios2::Mode::Write, comm,
                                            engineType, parameters,
                                            transportParameters);
    }
    else if (mode == "a")
    {
        m_Stream = std::make_shared<Stream>(name, adios2::Mode::Append, comm,
                                            engineType, parameters,
                                            transportParameters);
    }
    else
    {
        throw std::invalid_argument(
            "ERROR: adios2 mode " + mode + " for file " + name +
            " not supported, only \"r\", \"w\" and \"a\" (read, write, append) "
            "are valid modes, in call to open\n");
    }
    m_IsClosed = false;
}

FilePy::FilePy(const std::string &name, const std::string mode, MPI_Comm comm,
               const std::string configFile, const std::string ioInConfigFile)
: m_Name(name), m_Mode(mode)
{
    if (mode == "r")
    {
        m_Stream = std::make_shared<Stream>(name, adios2::Mode::Read, comm,
                                            configFile, ioInConfigFile);
    }
    else if (mode == "w")
    {
        m_Stream = std::make_shared<Stream>(name, adios2::Mode::Write, comm,
                                            configFile, ioInConfigFile);
    }
    else if (mode == "a")
    {
        m_Stream = std::make_shared<Stream>(name, adios2::Mode::Append, comm,
                                            configFile, ioInConfigFile);
    }
    else
    {
        throw std::invalid_argument("ERROR: adios2 mode " + mode +
                                    " for file " + name +
                                    " not supported, in call to open\n");
    }
    m_IsClosed = false;
}

FilePy::FilePy(const std::string &name, const std::string mode,
               const std::string engineType, const Params &parameters,
               const vParams &transportParameters)
: FilePy(name, mode, MPI_COMM_SELF, engineType, parameters, transportParameters)
{
}

FilePy::FilePy(const std::string &name, const std::string mode,
               const std::string configFile, const std::string ioInConfigFile)
: FilePy(name, mode, MPI_COMM_SELF, configFile, ioInConfigFile)
{
}

bool FilePy::eof() const
{
    bool eof = false;

    if (m_Stream->m_Status == StepStatus::EndOfStream)
    {
        eof = true;
    }

    return eof;
}

void FilePy::Write(const std::string &name, const pybind11::array &array,
                   const Dims &shape, const Dims &start, const Dims &count,
                   const bool endl)
{
    if (false)
    {
    }
#define declare_type(T)                                                        \
    else if (pybind11::isinstance<                                             \
                 pybind11::array_t<T, pybind11::array::c_style>>(array))       \
    {                                                                          \
        m_Stream->Write(name, reinterpret_cast<const T *>(array.data()),       \
                        shape, start, count, endl);                            \
    }
    ADIOS2_FOREACH_NUMPY_TYPE_1ARG(declare_type)
#undef declare_type
    else
    {
        throw std::invalid_argument(
            "ERROR: adios2 file write variable " + name +
            ", either numpy type is not supported or is"
            "c_style memory contiguous, in call to write\n");
    }
}

void FilePy::Write(const std::string &name, const pybind11::array &array,
                   const bool endl)
{
    Write(name, array, {}, {}, {}, endl);
}

void FilePy::Write(const std::string &name, const std::string &stringValue,
                   const bool endl)
{
    m_Stream->Write(name, stringValue, endl);
}

std::string FilePy::ReadString(const std::string &name, const bool endl)
{
    return m_Stream->Read<std::string>(name, endl).front();
}

std::string FilePy::ReadString(const std::string &name, const size_t step)
{
    std::string value;
    m_Stream->Read<std::string>(name, &value, Box<size_t>(step, 1));
    return value;
}

pybind11::array FilePy::Read(const std::string &name, const bool endl)
{
    const std::string type = m_Stream->m_IO->InquireVariableType(name);

    if (type == "string")
    {
        const std::string value =
            m_Stream->Read<std::string>(name, endl).front();
        pybind11::array pyArray(pybind11::dtype::of<char>(),
                                Dims{value.size()});
        char *pyPtr =
            reinterpret_cast<char *>(const_cast<void *>(pyArray.data()));
        std::copy(value.begin(), value.end(), pyPtr);
        return pyArray;
    }
#define declare_type(T)                                                        \
    else if (type == GetType<T>())                                             \
    {                                                                          \
        Variable<T> &variable = *m_Stream->m_IO->InquireVariable<T>(name);     \
        Dims count;                                                            \
        if (variable.m_SingleValue)                                            \
        {                                                                      \
            count = {1};                                                       \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            count = variable.m_Count;                                          \
        }                                                                      \
        pybind11::array pyArray(pybind11::dtype::of<T>(), count);              \
        m_Stream->Read<T>(                                                     \
            name, reinterpret_cast<T *>(const_cast<void *>(pyArray.data())),   \
            endl);                                                             \
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

pybind11::array FilePy::Read(const std::string &name,
                             const Dims &selectionStart,
                             const Dims &selectionCount, const bool endl)
{
    const std::string type = m_Stream->m_IO->InquireVariableType(name);

    if (type.empty())
    {
    }
#define declare_type(T)                                                        \
    else if (type == GetType<T>())                                             \
    {                                                                          \
        pybind11::array pyArray(pybind11::dtype::of<T>(), selectionCount);     \
        m_Stream->Read<T>(                                                     \
            name, reinterpret_cast<T *>(const_cast<void *>(pyArray.data())),   \
            Box<Dims>(selectionStart, selectionCount), endl);                  \
        return pyArray;                                                        \
    }
    ADIOS2_FOREACH_NUMPY_TYPE_1ARG(declare_type)
#undef declare_type

    throw std::invalid_argument(
        "ERROR: adios2 file read variable " + name +
        ", type can't be mapped to a numpy type, in call to read\n");
}

pybind11::array FilePy::Read(const std::string &name,
                             const Dims &selectionStart,
                             const Dims &selectionCount,
                             const size_t stepSelectionStart,
                             const size_t stepSelectionCount)
{
    const std::string type = m_Stream->m_IO->InquireVariableType(name);

    // shape of the returned numpy array
    Dims shapePy(selectionCount);
    std::transform(shapePy.begin(), shapePy.end(), shapePy.begin(),
                   std::bind1st(std::multiplies<size_t>(), stepSelectionCount));

    if (type.empty())
    {
    }
#define declare_type(T)                                                        \
    else if (type == GetType<T>())                                             \
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

void FilePy::Close()
{
    m_Stream->Close();
    m_IsClosed = true;
}

bool FilePy::IsClosed() const noexcept { return m_IsClosed; }

} // end namespace adios2
