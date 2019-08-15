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

namespace
{
Dims py_strides(const Dims &shape, ssize_t itemsize, bool hasStepDim)
{
    auto ndim = shape.size();
    Dims strides(ndim, itemsize);
    if (!hasStepDim)
    {
        // regular column-major
        for (size_t i = 1; i < ndim; ++i)
        {
            strides[i] = strides[i - 1] * shape[i - 1];
        }
    }
    else
    {
        // rotate so that slowest dim will be in front
        for (size_t i = 2; i < ndim; ++i)
        {
            strides[i] = strides[i - 1] * shape[i - 1];
        }
        strides[0] = strides[ndim - 1] * shape[ndim - 1];
    }
    return strides;
}
} // end anonymous namespace

template <class T>
pybind11::array File::DoRead(const std::string &name, const Dims &_start,
                             const Dims &_count, const size_t stepStart,
                             const size_t stepCount, const size_t blockID,
                             const std::string order)
{
    Layout layout = ToLayout(order);
    core::Variable<T> &variable = *m_Stream->m_IO->InquireVariable<T>(name);

    if (layout == Layout::Original)
    {
        layout = variable.GetOriginalLayout();
    }
    bool reverse_dims = layout == Layout::ColumnMajor;

    // Do everything in original dims first, ie, we need to
    // un-reverse shape and count we get from the core

    Dims shape = variable.m_Shape;
    if (reverse_dims)
    {
        std::reverse(shape.begin(), shape.end());
    }

    Dims start = _start;
    Dims count = _count;

    if (variable.m_ShapeID == ShapeID::GlobalValue)
    {
        if (!(_start.empty() && _count.empty()))
        {
            throw std::invalid_argument("when reading a scalar, start and "
                                        "count cannot be specified.\n");
        }
    }

    if (variable.m_ShapeID == ShapeID::LocalArray)
    {
        variable.SetBlockSelection(blockID);
    }
    else
    {
        if (blockID != 0)
        {
            throw std::invalid_argument(
                "blockId can only be specified when reading LocalArrays.");
        }
    }

    if (start.empty())
    {
        // default start to be (0, 0, ...)
        start = Dims(shape.size());
    }

    if (count.empty())
    {
        // does the right thing for global and local arrays
        count = variable.Count();
        if (reverse_dims)
        {
            std::reverse(count.begin(), count.end());
        }
    }

    // make numpy array, shape is count, possibly with extra dim for step
    // added
    Dims shapePy;
    if (stepCount == 0)
    {
        shapePy.reserve(count.size());
        std::copy(count.begin(), count.end(), std::back_inserter(shapePy));
    }
    else
    { // prepend step dimension
        shapePy.reserve(count.size() + 1);
        shapePy.emplace_back(stepCount);
        std::copy(count.begin(), count.end(), std::back_inserter(shapePy));
    }

    pybind11::array_t<T> pyArray;
    if (layout == Layout::ColumnMajor)
    {
        pyArray = pybind11::array_t<T>(
            shapePy, py_strides(shapePy, pyArray.itemsize(), stepCount > 0));
    }
    else
    {
        pyArray = pybind11::array_t<T, pybind11::array::c_style>(shapePy);
    }

    // set selection if requested
    if (!start.empty() && !count.empty())
    {
        if (reverse_dims)
        {
            std::reverse(start.begin(), start.end());
            std::reverse(count.begin(), count.end());
        }
        variable.SetSelection(Box<Dims>(std::move(start), std::move(count)));
    }

    // set step selection if requested
    if (stepCount > 0)
    {
        variable.SetStepSelection({stepStart, stepCount});
    }

    if (!m_Stream->m_Engine)
    {
        throw std::logic_error("no engine available in DoRead()");
    }
    m_Stream->m_Engine->Get(variable, pyArray.mutable_data(), Mode::Sync);

    return pyArray;
}

} // end namespace py11
} // end namespace adios2

#endif
