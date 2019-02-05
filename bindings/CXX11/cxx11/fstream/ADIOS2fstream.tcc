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
                              const std::string separator)
{
    m_Stream->WriteAttribute(name, value, variableName, separator);
}

template <class T>
void fstream::write_attribute(const std::string &name, const T *data,
                              const size_t elements,
                              const std::string &variableName,
                              const std::string separator)
{
    m_Stream->WriteAttribute(name, data, elements, variableName, separator);
}

template <class T>
void fstream::write(const std::string &name, const T *values, const Dims &shape,
                    const Dims &start, const Dims &count, const bool endl)
{
    m_Stream->Write(name, values, shape, start, count, endl);
}

template <class T>
void fstream::write(const std::string &name, const T &value, const bool endl)
{
    m_Stream->Write(name, value, endl);
}

template <class T>
void fstream::read(const std::string &name, T *values)
{
    m_Stream->Read(name, values);
}

template <class T>
void fstream::read(const std::string &name, T &value)
{
    m_Stream->Read(name, &value);
}

template <class T>
void fstream::read(const std::string &name, T *values,
                   const Dims &selectionStart, const Dims &selectionCount)
{
    m_Stream->Read(name, values, Box<Dims>(selectionStart, selectionCount));
}

template <class T>
void fstream::read(const std::string &name, T *values,
                   const Dims &selectionStart, const Dims &selectionCount,
                   const size_t stepSelectionStart,
                   const size_t stepSelectionCount)
{
    m_Stream->Read(name, values, Box<Dims>(selectionStart, selectionCount),
                   Box<size_t>(stepSelectionStart, stepSelectionCount));
}

template <class T>
void fstream::read(const std::string &name, T &value,
                   const size_t stepSelectionStart)
{
    m_Stream->Read(name, &value, Box<size_t>(stepSelectionStart, 1));
}

template <class T>
std::vector<T> fstream::read(const std::string &name)
{
    return m_Stream->Read<T>(name);
}

template <class T>
std::vector<T> fstream::read(const std::string &name,
                             const Dims &selectionStart,
                             const Dims &selectionCount)
{
    return m_Stream->Read<T>(name, Box<Dims>(selectionStart, selectionCount));
}

template <class T>
std::vector<T>
fstream::read(const std::string &name, const Dims &selectionStart,
              const Dims &selectionCount, const size_t stepSelectionStart,
              const size_t stepSelectionCount)
{
    return m_Stream->Read<T>(
        name, Box<Dims>(selectionStart, selectionCount),
        Box<size_t>(stepSelectionStart, stepSelectionCount));
}

template <class T>
std::vector<T> fstream::read_attribute(const std::string &name,
                                       const std::string &variableName,
                                       const std::string separator)
{
    return m_Stream->ReadAttribute<T>(name, variableName, separator);
}

// template <class T>
// void fstream::read(const std::string &name, T *values,
//                   const size_t stepSelectionStart,
//                   const size_t stepSelectionCount)
//{
//    // TODO
//    throw std::invalid_argument("ERROR: fstream read variable " + name +
//                                " signature not yet implemented\n");
//}
//
// template <class T>
// T fstream::read(const std::string &name, const size_t step)
//{
//    // TODO
//    throw std::invalid_argument("ERROR: fstream read variable " + name +
//                                " signature not yet implemented\n");
//}
//
// template <class T>
// std::vector<T> fstream::read(const std::string &name,
//                             const size_t stepSelectionStart,
//                             const size_t stepSelectionCount)
//{
//    // TODO
//    throw std::invalid_argument("ERROR: fstream read variable " + name +
//                                " signature not yet implemented\n");
//}

} // end namespace adios2

#endif /* ADIOS2_BINDINGS_CXX11_CXX11_FSTREAM_ADIOS2FSTREAM_TCC_ */
