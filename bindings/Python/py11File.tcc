/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * py11File.tcc
 *
 *  Created on: May 29, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_PYTHON_PY11FILE_TCC_
#define ADIOS2_BINDINGS_PYTHON_PY11FILE_TCC_

#include "py11File.h"

namespace adios2
{
namespace py11
{

template <class T>
pybind11::array File::DoRead(const std::string &name, const Dims &_start,
                             const Dims &_count, const size_t blockID)
{
    core::Variable<T> &variable = *m_Stream->m_IO->InquireVariable<T>(name);
    Dims &shape = variable.m_Shape;

    Dims start = _start;
    if (start.empty())
    {
        // default start to be (0, 0, ...)
        start = Dims(shape.size());
    }

    Dims count = _count;
    if (count.empty())
    {
        if (variable.m_ShapeID == ShapeID::GlobalArray)
        {
            // default count is everything (shape of whole array)
            count = shape;
        }
        else if (variable.m_ShapeID == ShapeID::LocalArray)
        {
            variable.SetBlockSelection(blockID);
            count = variable.Count();
        }
    }

    if (variable.m_ShapeID == ShapeID::GlobalValue)
    {
        count = Dims{1};
    }
    pybind11::array_t<T> pyArray(count);
    if (variable.m_ShapeID == ShapeID::GlobalValue)
    {
        if (!(_start.empty() && _count.empty()))
        {
            throw std::invalid_argument("when reading a scalar, start and "
                                        "count cannot be specified.\n");
        }
        m_Stream->Read<T>(name, pyArray.mutable_data(), blockID);
    }
    else
    {
        m_Stream->Read<T>(name, pyArray.mutable_data(),
                          Box<Dims>(std::move(start), std::move(count)),
                          blockID);
    }
    return pyArray;
}

template <class T>
pybind11::array File::DoRead(const std::string &name, const Dims &start,
                             const Dims &count, const size_t stepStart,
                             const size_t stepCount, const size_t blockID)
{
    // shape of the returned numpy array
    Dims shapePy(count.size() + 1);
    shapePy[0] = stepCount;
    for (auto i = 1; i < shapePy.size(); ++i)
    {
        shapePy[i] = count[i - 1];
    }

    pybind11::array_t<T> pyArray(shapePy);
    m_Stream->Read<T>(name, pyArray.mutable_data(), Box<Dims>(start, count),
                      Box<size_t>(stepStart, stepCount), blockID);
    return pyArray;
}

} // end namespace py11
} // end namespace adios2

#endif
