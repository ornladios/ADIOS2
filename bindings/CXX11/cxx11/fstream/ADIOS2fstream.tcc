/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2fstream.tcc
 *
 *  Created on: Mar 5, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_CXX11_CXX11_FSTREAM_ADIOS2FSTREAM_TCC_
#define ADIOS2_BINDINGS_CXX11_CXX11_FSTREAM_ADIOS2FSTREAM_TCC_

#include "ADIOS2fstream.h"

#include "adios2/core/Stream.h"

namespace adios2
{

template <class T>
void fstream::write_attribute(const std::string &name, const T &value,
                              const std::string &variableName,
                              const std::string separator, const bool endl)
{
    using IOType = typename TypeInfo<T>::IOType;
    m_Stream->WriteAttribute(name, reinterpret_cast<const IOType &>(value),
                             variableName, separator, endl);
}

template <class T>
void fstream::write_attribute(const std::string &name, const T *data,
                              const size_t elements,
                              const std::string &variableName,
                              const std::string separator, const bool endl)
{
    using IOType = typename TypeInfo<T>::IOType;
    m_Stream->WriteAttribute(name, reinterpret_cast<const IOType *>(data),
                             elements, variableName, separator, endl);
}

template <class T>
void fstream::write(const std::string &name, const T *values, const Dims &shape,
                    const Dims &start, const Dims &count, const bool endl)
{
    using IOType = typename TypeInfo<T>::IOType;
    m_Stream->Write(name, reinterpret_cast<const IOType *>(values), shape,
                    start, count, endl);
}

template <class T>
void fstream::write(const std::string &name, const T &value, const bool endl)
{
    using IOType = typename TypeInfo<T>::IOType;
    m_Stream->Write(name, reinterpret_cast<const IOType &>(value), endl);
}

template <class T>
void fstream::read(const std::string &name, T *values)
{
    using IOType = typename TypeInfo<T>::IOType;
    m_Stream->Read(name, reinterpret_cast<IOType *>(values));
}

template <class T>
void fstream::read(const std::string &name, T *values, const size_t stepStart,
                   const size_t stepCount)
{
    using IOType = typename TypeInfo<T>::IOType;
    m_Stream->Read(name, reinterpret_cast<IOType *>(values),
                   Box<size_t>{stepStart, stepCount});
}

template <class T>
void fstream::read(const std::string &name, T &value)
{
    using IOType = typename TypeInfo<T>::IOType;
    m_Stream->Read(name, &reinterpret_cast<IOType &>(value));
}

template <class T>
void fstream::read(const std::string &name, T *values,
                   const Dims &selectionStart, const Dims &selectionCount)
{
    using IOType = typename TypeInfo<T>::IOType;
    m_Stream->Read(name, reinterpret_cast<IOType *>(values),
                   Box<Dims>(selectionStart, selectionCount));
}

template <class T>
void fstream::read(const std::string &name, T *values,
                   const Dims &selectionStart, const Dims &selectionCount,
                   const size_t stepSelectionStart,
                   const size_t stepSelectionCount)
{
    using IOType = typename TypeInfo<T>::IOType;
    m_Stream->Read(name, reinterpret_cast<IOType *>(values),
                   Box<Dims>(selectionStart, selectionCount),
                   Box<size_t>(stepSelectionStart, stepSelectionCount));
}

template <class T>
void fstream::read(const std::string &name, T &value,
                   const size_t stepSelectionStart)
{
    using IOType = typename TypeInfo<T>::IOType;
    m_Stream->Read(name, &reinterpret_cast<IOType &>(value),
                   Box<size_t>(stepSelectionStart, 1));
}

template <class T>
std::vector<T> fstream::read(const std::string &name)
{
    using IOType = typename TypeInfo<T>::IOType;
    auto vec = m_Stream->Read<IOType>(name);
    return reinterpret_cast<std::vector<T> &>(vec);
}

template <class T>
std::vector<T> fstream::read(const std::string &name, const size_t stepsStart,
                             const size_t stepsCount)
{
    using IOType = typename TypeInfo<T>::IOType;
    auto vec =
        m_Stream->Read<IOType>(name, Box<size_t>(stepsStart, stepsCount));
    return reinterpret_cast<std::vector<T> &>(vec);
}

template <class T>
std::vector<T> fstream::read(const std::string &name,
                             const Dims &selectionStart,
                             const Dims &selectionCount)
{
    using IOType = typename TypeInfo<T>::IOType;
    auto vec =
        m_Stream->Read<IOType>(name, Box<Dims>(selectionStart, selectionCount));
    return reinterpret_cast<std::vector<T> &>(vec);
}

template <class T>
std::vector<T>
fstream::read(const std::string &name, const Dims &selectionStart,
              const Dims &selectionCount, const size_t stepSelectionStart,
              const size_t stepSelectionCount)
{
    using IOType = typename TypeInfo<T>::IOType;
    auto vec = m_Stream->Read<IOType>(
        name, Box<Dims>(selectionStart, selectionCount),
        Box<size_t>(stepSelectionStart, stepSelectionCount));
    return reinterpret_cast<std::vector<T> &>(vec);
}

template <class T>
std::vector<T> fstream::read_attribute(const std::string &name,
                                       const std::string &variableName,
                                       const std::string separator)
{
    using IOType = typename TypeInfo<T>::IOType;
    std::vector<T> data;
    core::Attribute<IOType> *attribute =
        m_Stream->m_IO->InquireAttribute<IOType>(name);

    if (attribute == nullptr)
    {
        return data;
    }

    data.resize(attribute->m_Elements);
    m_Stream->ReadAttribute(name, reinterpret_cast<IOType *>(data.data()),
                            variableName, separator);
    return data;
}

} // end namespace adios2

#endif /* ADIOS2_BINDINGS_CXX11_CXX11_FSTREAM_ADIOS2FSTREAM_TCC_ */
