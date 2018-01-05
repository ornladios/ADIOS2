/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2fstream.tcc
 *
 *  Created on: Mar 5, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "ADIOS2fstream.h"

namespace adios2
{

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
void fstream::read(const std::string &name, T *values, const bool endl)
{
    m_Stream->Read(name, values, endl);
}

template <class T>
void fstream::read(const std::string &name, T &value, const bool endl)
{
    m_Stream->Read(name, &value, endl);
}

template <class T>
void fstream::read(const std::string &name, T *values,
                   const Dims &selectionStart, const Dims &selectionCount,
                   const bool endl)
{
    m_Stream->Read(name, values, Box<Dims>(selectionStart, selectionCount),
                   endl);
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
std::vector<T> fstream::read(const std::string &name, const bool endl)
{
    return m_Stream->Read<T>(name, endl);
}

template <class T>
std::vector<T> fstream::read(const std::string &name,
                             const Dims &selectionStart,
                             const Dims &selectionCount, const bool endl)
{
    return m_Stream->Read<T>(name, Box<Dims>(selectionStart, selectionCount),
                             endl);
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
void fstream::read(const std::string &name, T *values,
                   const size_t stepSelectionStart,
                   const size_t stepSelectionCount)
{
    // TODO
    throw std::invalid_argument("ERROR: fstream read variable " + name +
                                " signature not yet implemented\n");
}

template <class T>
T fstream::read(const std::string &name, const size_t step)
{
    // TODO
    throw std::invalid_argument("ERROR: fstream read variable " + name +
                                " signature not yet implemented\n");
}

template <class T>
std::vector<T> fstream::read(const std::string &name,
                             const size_t stepSelectionStart,
                             const size_t stepSelectionCount)
{
    // TODO
    throw std::invalid_argument("ERROR: fstream read variable " + name +
                                " signature not yet implemented\n");
}

} // end namespace adios2
