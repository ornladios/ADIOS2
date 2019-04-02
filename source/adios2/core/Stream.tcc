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

#include "Stream.h"

#include "adios2/core/Variable.h"

namespace adios2
{
namespace core
{

template <class T>
void Stream::WriteAttribute(const std::string &name, const T &value,
                            const std::string &variableName,
                            const std::string separator, const bool nextStep)
{
    m_IO->DefineAttribute<T>(name, value, variableName, separator);
    CheckOpen();
    if (!m_StepStatus)
    {
        m_Engine->BeginStep();
        m_StepStatus = true;
    }

    if (nextStep)
    {
        m_Engine->EndStep();
        m_StepStatus = false;
    }
}

template <class T>
void Stream::WriteAttribute(const std::string &name, const T *array,
                            const size_t elements,
                            const std::string &variableName,
                            const std::string separator, const bool nextStep)
{
    m_IO->DefineAttribute<T>(name, array, elements, variableName, separator);
    CheckOpen();
    if (!m_StepStatus)
    {
        m_Engine->BeginStep();
        m_StepStatus = true;
    }

    if (nextStep)
    {
        m_Engine->EndStep();
        m_StepStatus = false;
    }
}

template <class T>
void Stream::Write(const std::string &name, const T *data, const Dims &shape,
                   const Dims &start, const Dims &count, const bool nextStep)
{
    Variable<T> *variable = m_IO->InquireVariable<T>(name);

    if (variable == nullptr)
    {
        variable = &m_IO->DefineVariable<T>(name, shape, start, count, false);
    }
    else
    {
        if (!shape.empty() && !variable->m_SingleValue)
        {
            variable->SetShape(shape);
        }

        if (!start.empty() && !count.empty())
        {
            variable->SetSelection(Box<Dims>(start, count));
        }
    }

    CheckOpen();
    if (!m_StepStatus)
    {
        m_Engine->BeginStep();
        m_StepStatus = true;
    }

    m_Engine->Put(*variable, data, adios2::Mode::Sync);

    if (nextStep)
    {
        m_Engine->EndStep();
        m_StepStatus = false;
    }
}

template <class T>
void Stream::Write(const std::string &name, const T &datum, const bool nextStep)
{
    const T datumLocal = datum;
    Write(name, &datumLocal, {}, {}, {}, nextStep);
}

template <class T>
void Stream::Read(const std::string &name, T *values)
{
    CheckPCommon(name, values);

    Variable<T> *variable = m_IO->InquireVariable<T>(name);
    if (variable == nullptr)
    {
        values = nullptr;
        return;
    }

    GetPCommon(*variable, values);
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
    GetPCommon(*variable, values);
}

template <class T>
void Stream::Read(const std::string &name, T *values,
                  const Box<Dims> &selection)
{
    CheckPCommon(name, values);

    Variable<T> *variable = m_IO->InquireVariable<T>(name);
    if (variable == nullptr)
    {
        values = nullptr;
        return;
    }

    variable->SetSelection(selection);
    GetPCommon(*variable, values);
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
    GetPCommon(*variable, values);
}

template <class T>
std::vector<T> Stream::Read(const std::string &name)
{
    Variable<T> *variable = m_IO->InquireVariable<T>(name);
    if (variable == nullptr)
    {
        return std::vector<T>();
    }
    return GetCommon(*variable);
}

template <class T>
std::vector<T> Stream::Read(const std::string &name,
                            const Box<size_t> &stepsSelection)
{
    Variable<T> *variable = m_IO->InquireVariable<T>(name);
    if (variable == nullptr)
    {
        return std::vector<T>();
    }
    variable->SetStepSelection(stepsSelection);
    return GetCommon(*variable);
}

template <class T>
std::vector<T> Stream::Read(const std::string &name, const Box<Dims> &selection)
{
    Variable<T> *variable = m_IO->InquireVariable<T>(name);
    if (variable == nullptr)
    {
        return std::vector<T>();
    }

    variable->SetSelection(selection);
    return GetCommon(*variable);
}

template <class T>
std::vector<T> Stream::Read(const std::string &name, const Box<Dims> &selection,
                            const Box<size_t> &stepSelection)
{
    Variable<T> *variable = m_IO->InquireVariable<T>(name);
    if (variable == nullptr)
    {
        return std::vector<T>();
    }

    variable->SetSelection(selection);
    variable->SetStepSelection(stepSelection);
    return GetCommon(*variable);
}

template <class T>
void Stream::ReadAttribute(const std::string &name, T *data,
                           const std::string &variableName,
                           const std::string separator)
{
    Attribute<T> *attribute =
        m_IO->InquireAttribute<T>(name, variableName, separator);

    if (attribute == nullptr)
    {
        return;
    }

    if (attribute->m_IsSingleValue)
    {
        data[0] = attribute->m_DataSingleValue;
    }
    else
    {
        std::copy(attribute->m_DataArray.begin(), attribute->m_DataArray.end(),
                  data);
    }
}

// PRIVATE
template <class T>
std::vector<T> Stream::GetCommon(Variable<T> &variable)
{
    try
    {
        std::vector<T> values(variable.SelectionSize());
        CheckOpen();
        m_Engine->Get(variable, values.data(), adios2::Mode::Sync);
        return values;
    }
    catch (std::exception &e)
    {
        std::throw_with_nested(
            std::runtime_error("ERROR: couldn't read variable " +
                               variable.m_Name + "\n" + e.what()));
    }
}

template <class T>
void Stream::GetPCommon(Variable<T> &variable, T *values)
{
    try
    {
        CheckOpen();
        m_Engine->Get(variable, values, adios2::Mode::Sync);
    }
    catch (std::exception &e)
    {
        std::throw_with_nested(
            std::runtime_error("ERROR: couldn't read pointer variable " +
                               variable.m_Name + "\n" + e.what()));
    }
}

template <class T>
void Stream::CheckPCommon(const std::string &name, const T *values) const
{
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
