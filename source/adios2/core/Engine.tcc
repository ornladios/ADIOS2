/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Engine.tcc
 *
 *  Created on: Jun 2, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_CORE_ENGINE_TCC_
#define ADIOS2_CORE_ENGINE_TCC_

#include "Engine.h"

#include <stdexcept>

#include "adios2/helper/adiosFunctions.h" // CheckforNullptr

namespace adios2
{
namespace core
{

template <class T>
void Engine::Put(Variable<T> &variable, const T *data, const Mode launch)
{
    if (m_DebugMode)
    {
        CommonChecks(variable, data, {{Mode::Write, Mode::Append}},
                     "in call to Put");
    }

    switch (launch)
    {
    case Mode::Deferred:
        DoPutDeferred(variable, data);
        break;
    case Mode::Sync:
        DoPutSync(variable, data);
        break;
    default:
        if (m_DebugMode)
        {
            throw std::invalid_argument(
                "ERROR: invalid launch Mode for variable " + variable.m_Name +
                ", only Mode::Deferred and Mode::Sync are valid, in call to "
                "Put\n");
        }
    }
}

template <class T>
void Engine::Put(const std::string &variableName, const T *data,
                 const Mode launch)
{
    Put(FindVariable<T>(variableName, "in call to Put"), data, launch);
}

template <class T>
void Engine::Put(Variable<T> &variable, const T &datum)
{
    const T datumLocal = datum;
    Put(variable, &datumLocal, Mode::Sync);
}

template <class T>
void Engine::Put(const std::string &variableName, const T &datum)
{
    Put(FindVariable<T>(variableName, "in call to Put"), datum);
}

// Get
template <class T>
void Engine::Get(Variable<T> &variable, T *data, const Mode launch)
{
    if (m_DebugMode)
    {
        CommonChecks(variable, data, {{Mode::Read}}, "in call to Get");
    }

    switch (launch)
    {
    case Mode::Deferred:
        DoGetDeferred(variable, data);
        break;
    case Mode::Sync:
        DoGetSync(variable, data);
        break;
    default:
        if (m_DebugMode)
        {
            throw std::invalid_argument(
                "ERROR: invalid launch Mode for variable " + variable.m_Name +
                ", only Mode::Deferred and Mode::Sync are valid, in call to "
                "Get\n");
        }
    }
}

template <class T>
void Engine::Get(const std::string &variableName, T *data, const Mode launch)
{
    Get(FindVariable<T>(variableName, "in call to Get"), data, launch);
}

template <class T>
void Engine::Get(Variable<T> &variable, T &datum, const Mode /*launch*/)
{
    Get(variable, &datum, Mode::Sync);
}

template <class T>
void Engine::Get(const std::string &variableName, T &datum, const Mode launch)
{
    Get(FindVariable<T>(variableName, "in call to Get"), datum, launch);
}

template <class T>
void Engine::Get(Variable<T> &variable, std::vector<T> &dataV,
                 const Mode launch)
{
    const size_t dataSize = variable.SelectionSize();
    helper::Resize(dataV, dataSize, m_DebugMode,
                   "in call to Get with std::vector argument");
    Get(variable, dataV.data(), launch);
}

template <class T>
void Engine::Get(const std::string &variableName, std::vector<T> &dataV,
                 const Mode launch)
{
    Get(FindVariable<T>(variableName, "in Get with std::vector argument"),
        dataV, launch);
}

template <class T>
std::map<size_t, std::vector<typename Variable<T>::Info>>
Engine::AllStepsBlocksInfo(const Variable<T> &variable) const
{
    return DoAllStepsBlocksInfo(variable);
}

template <class T>
std::vector<typename Variable<T>::Info>
Engine::BlocksInfo(const Variable<T> &variable, const size_t step) const
{
    return DoBlocksInfo(variable, step);
}

// PROTECTED
template <class T>
Variable<T> &Engine::FindVariable(const std::string &variableName,
                                  const std::string hint)
{
    Variable<T> *variable = m_IO.InquireVariable<T>(variableName);
    if (m_DebugMode)
    {
        if (variable == nullptr)
        {
            throw std::invalid_argument("ERROR: variable " + variableName +
                                        " not found in IO " + m_IO.m_Name +
                                        ", " + hint + "\n");
        }
    }
    return *variable;
}

// PRIVATE
template <class T>
void Engine::CommonChecks(Variable<T> &variable, const T *data,
                          const std::set<Mode> &modes,
                          const std::string hint) const
{
    helper::CheckForNullptr(&variable, "for variable argument, " + hint);
    variable.CheckDimensions(hint);
    CheckOpenModes(modes, " for variable " + variable.m_Name + ", " + hint);

    const bool zeros =
        std::all_of(variable.m_Count.begin(), variable.m_Count.end(),
                    [](const size_t d) { return d == 0; });
    if (!zeros)
    {
        helper::CheckForNullptr(
            data, "for data argument in non-zero count block, " + hint);
    }

    if (!variable.m_MemoryStart.empty() && !variable.m_MemoryCount.empty())
    {
        const Box<Dims> selectionBox =
            helper::StartEndBox(variable.m_Start, variable.m_Count);
        const Box<Dims> memoryBox =
            helper::StartEndBox(variable.m_MemoryStart, variable.m_MemoryCount);

        const Box<Dims> intersectionBox =
            helper::IntersectionBox(selectionBox, memoryBox);
        if (intersectionBox.first != selectionBox.first ||
            intersectionBox.second != selectionBox.second)
        {
            throw std::invalid_argument(
                "ERROR: variable start and count do not fall inside "
                "SetMemorySelection start and count for variable " +
                variable.m_Name + ", " + hint);
        }
    }
}

} // end namespace core
} // end namespace adios2

#endif /** ADIOS2_CORE_ENGINE_TCC_ */
