/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Stream.tcc
 *
 *  Created on: Jan 5, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_CORE_STREAM_TCC_
#define ADIOS2_CORE_STREAM_TCC_

#include "adios2/core/Variable.h"
#include <adios2/core/Stream.h>

namespace adios2
{
namespace core
{

template <class T>
void Stream::Write(const std::string &name, const T *data, const Dims &shape,
                   const Dims &start, const Dims &count, const bool endStep)
{
    ThrowIfNotOpen("variable " + name + ", in call to write\n");

    Variable<T> *variable = m_IO->InquireVariable<T>(name);

    if (variable == nullptr)
    {
        variable = &m_IO->DefineVariable<T>(name, shape, start, count, false);
    }
    else
    {
        if (!shape.empty())
        {
            variable->SetShape(shape);
        }

        if (!start.empty() && !count.empty())
        {
            variable->SetSelection(Box<Dims>(start, count));
        }
    }

    m_Engine->Put(*variable, data, adios2::Mode::Sync);

    if (endStep)
    {
        m_Engine->EndStep();
        m_Status = m_Engine->BeginStep();
    }
}

template <class T>
void Stream::Write(const std::string &name, const T &datum, const bool endStep)
{
    const T datumLocal = datum;
    Write(name, &datumLocal, {}, {}, {}, endStep);
}

template <class T>
void Stream::Read(const std::string &name, T *values, const bool endStep)
{
    CheckPCommon(name, values);

    Variable<T> *variable = m_IO->InquireVariable<T>(name);
    if (variable == nullptr)
    {
        values = nullptr;
        return;
    }

    variable->SetStepSelection({m_Engine->CurrentStep(), 1});
    GetPCommon(*variable, values, endStep);
}

template <class T>
void Stream::Read(const std::string &name, T *values,
                  const Box<size_t> &stepSelection)
{
    CheckPCommon(name, values);

    Variable<T> *variable = m_IO->InquireVariable<T>(name);
    if (variable == nullptr)
    {
        values = nullptr;
        return;
    }

    variable->SetStepSelection(stepSelection);
    GetPCommon(*variable, values, false);
}

template <class T>
void Stream::Read(const std::string &name, T *values,
                  const Box<Dims> &selection, const bool endStep)
{
    CheckPCommon(name, values);

    Variable<T> *variable = m_IO->InquireVariable<T>(name);
    if (variable == nullptr)
    {
        values = nullptr;
        return;
    }

    variable->SetSelection(selection);
    GetPCommon(*variable, values, endStep);
}

template <class T>
void Stream::Read(const std::string &name, T *values,
                  const Box<Dims> &selection, const Box<size_t> &stepSelection)
{
    CheckPCommon(name, values);

    Variable<T> *variable = m_IO->InquireVariable<T>(name);
    if (variable == nullptr)
    {
        values = nullptr;
        return;
    }

    variable->SetSelection(selection);
    variable->SetStepSelection(stepSelection);
    GetPCommon(*variable, values, false);
}

template <class T>
std::vector<T> Stream::Read(const std::string &name, const bool endStep)
{
    ThrowIfNotOpen("variable " + name + ", in call to read\n");
    Variable<T> *variable = m_IO->InquireVariable<T>(name);
    if (variable == nullptr)
    {
        return std::vector<T>();
    }

    variable->SetStepSelection({m_Engine->CurrentStep(), 1});
    return GetCommon(*variable, endStep);
}

// template <class T>
// std::vector<T> Stream::Read(const std::string &name,
//                            const Box<size_t> &stepSelection)
//{
//    // TODO
//}

template <class T>
std::vector<T> Stream::Read(const std::string &name, const Box<Dims> &selection,
                            const bool endStep)
{
    ThrowIfNotOpen("variable " + name + ", in call to read\n");
    Variable<T> *variable = m_IO->InquireVariable<T>(name);
    if (variable == nullptr)
    {
        return std::vector<T>();
    }

    variable->SetSelection(selection);
    variable->SetStepSelection({m_Engine->CurrentStep(), 1});
    return GetCommon(*variable, endStep);
}

template <class T>
std::vector<T> Stream::Read(const std::string &name, const Box<Dims> &selection,
                            const Box<size_t> &stepSelection)
{
    ThrowIfNotOpen("variable " + name + ", in call to read\n");
    Variable<T> *variable = m_IO->InquireVariable<T>(name);
    if (variable == nullptr)
    {
        return std::vector<T>();
    }

    variable->SetSelection(selection);
    variable->SetStepSelection(stepSelection);
    return GetCommon(*variable, false);
}

// PRIVATE
template <class T>
std::vector<T> Stream::GetCommon(Variable<T> &variable, const bool endStep)
{
    try
    {
        std::vector<T> values(variable.SelectionSize());
        m_Engine->Get(variable, values.data(), adios2::Mode::Sync);
        if (endStep)
        {
            m_Engine->EndStep();
            m_Status = m_Engine->BeginStep();
        }
        return values;
    }
    catch (...)
    {
        std::throw_with_nested(std::runtime_error(
            "ERROR: couldn't Read variable " + variable.m_Name + "\n"));
    }
}

template <class T>
void Stream::GetPCommon(Variable<T> &variable, T *values, const bool endStep)
{
    try
    {
        m_Engine->Get(variable, values, adios2::Mode::Sync);
        if (endStep)
        {
            m_Engine->EndStep();
            m_Status = m_Engine->BeginStep();
        }
    }
    catch (...)
    {
        std::throw_with_nested(std::runtime_error(
            "ERROR: couldn't Read pointer variable " + variable.m_Name + "\n"));
    }
}

template <class T>
void Stream::CheckPCommon(const std::string &name, const T *values) const
{
    ThrowIfNotOpen("variable " + name + ", in call to read pointer\n");

    if (values == nullptr)
    {
        throw std::runtime_error(
            "ERROR: passed null values pointer for variable " + name +
            ", in call to read pointer\n");
    }
}

} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_CORE_STREAM_TCC_ */
