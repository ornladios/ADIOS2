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
                             const Dims &_count, const size_t stepStart,
                             const size_t stepCount, const size_t blockID)
{
    core::Variable<T> &variable = *m_Stream->m_IO->InquireVariable<T>(name);
    Dims &shape = variable.m_Shape;
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
    }

    // make numpy array, shape is count, possibly with extra dim for step added
    Dims shapePy;
    shapePy.reserve((stepCount > 0 ? 1 : 0) + count.size());
    if (stepCount > 0)
    {
        shapePy.emplace_back(stepCount);
    }
    std::copy(count.begin(), count.end(), std::back_inserter(shapePy));

    pybind11::array_t<T> pyArray(shapePy);

    // set selection if requested
    if (!start.empty() && !count.empty())
    {
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

    return std::move(pyArray);
}

} // end namespace py11
} // end namespace adios2

#endif
