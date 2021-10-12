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

#include "adios2/engine/inline/InlineReader.h"
#include "adios2/helper/adiosFunctions.h" // CheckforNullptr

namespace adios2
{
namespace core
{

template <class T>
typename Variable<T>::Span &Engine::Put(Variable<T> &variable,
                                        const bool initialize, const T &value)
{
    CheckOpenModes({{Mode::Write}}, " for variable " + variable.m_Name +
                                        ", in call to Variable<T>::Span Put");

    auto itSpan = variable.m_BlocksSpan.emplace(
        variable.m_BlocksInfo.size(),
        typename Variable<T>::Span(*this, variable.TotalSize()));
    DoPut(variable, itSpan.first->second, initialize, value);
    return itSpan.first->second;
}

template <class T>
void Engine::Put(Variable<T> &variable, const T *data, const Mode launch)
{
    CommonChecks(variable, data, {{Mode::Write, Mode::Append}},
                 "in call to Put");

    switch (launch)
    {
    case Mode::Deferred:
        DoPutDeferred(variable, data);
        break;
    case Mode::Sync:
        DoPutSync(variable, data);
        break;
    default:
        throw std::invalid_argument(
            "ERROR: invalid launch Mode for variable " + variable.m_Name +
            ", only Mode::Deferred and Mode::Sync are valid, in call to "
            "Put\n");
    }
}

template <class T>
void Engine::Put(const std::string &variableName, const T *data,
                 const Mode launch)
{
    Put(FindVariable<T>(variableName, "in call to Put"), data, launch);
}

template <class T>
void Engine::Put(Variable<T> &variable, const T &datum, const Mode /*launch*/)
{
    const T datumLocal = datum;
    Put(variable, &datumLocal, Mode::Sync);
}

template <class T>
void Engine::Put(const std::string &variableName, const T &datum,
                 const Mode /*launch*/)
{
    const T datumLocal = datum;
    Put(FindVariable<T>(variableName, "in call to Put"), &datumLocal,
        Mode::Sync);
}

// Get
template <class T>
void Engine::Get(Variable<T> &variable, T *data, const Mode launch)
{
    CommonChecks(variable, data, {{Mode::Read}, {Mode::ReadRandomAccess}},
                 "in call to Get");

    switch (launch)
    {
    case Mode::Deferred:
        DoGetDeferred(variable, data);
        break;
    case Mode::Sync:
        DoGetSync(variable, data);
        break;
    default:
        throw std::invalid_argument(
            "ERROR: invalid launch Mode for variable " + variable.m_Name +
            ", only Mode::Deferred and Mode::Sync are valid, in call to "
            "Get\n");
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
    helper::Resize(dataV, dataSize, "in call to Get with std::vector argument");
    Get(variable, dataV.data(), launch);
}

template <class T>
void Engine::Get(core::Variable<T> &variable, T **data) const
{
    const auto *eng =
        dynamic_cast<const adios2::core::engine::InlineReader *>(this);
    if (eng)
    {
        eng->Get(variable, data);
    }
    else
    {
        throw std::runtime_error("Currently, only the inline engine implements "
                                 "Get(core::Variable<T>&, T**)");
    }
}

template <class T>
void Engine::Get(const std::string &variableName, std::vector<T> &dataV,
                 const Mode launch)
{
    Get(FindVariable<T>(variableName, "in Get with std::vector argument"),
        dataV, launch);
}

// Get
template <class T>
typename Variable<T>::BPInfo *Engine::Get(Variable<T> &variable,
                                          const Mode launch)
{
    typename Variable<T>::BPInfo *info = nullptr;
    switch (launch)
    {
    case Mode::Deferred:
        info = DoGetBlockDeferred(variable);
        break;
    case Mode::Sync:
        info = DoGetBlockSync(variable);
        break;
    default:
        throw std::invalid_argument(
            "ERROR: invalid launch Mode for variable " + variable.m_Name +
            ", only Mode::Deferred and Mode::Sync are valid, in call to "
            "GetBlock\n");
    }

    CommonChecks<T>(variable, info->Data, {{Mode::Read}}, "in call to Get");

    return info;
}

template <class T>
typename Variable<T>::BPInfo *Engine::Get(const std::string &variableName,
                                          const Mode launch)
{
    return Get(FindVariable<T>(variableName, "in call to Get"), launch);
}

template <class T>
std::map<size_t, std::vector<typename Variable<T>::BPInfo>>
Engine::AllStepsBlocksInfo(const Variable<T> &variable) const
{
    return DoAllStepsBlocksInfo(variable);
}

template <class T>
std::vector<std::vector<typename Variable<T>::BPInfo>>
Engine::AllRelativeStepsBlocksInfo(const Variable<T> &variable) const
{
    return DoAllRelativeStepsBlocksInfo(variable);
}

template <class T>
std::vector<typename Variable<T>::BPInfo>
Engine::BlocksInfo(const Variable<T> &variable, const size_t step) const
{
    return DoBlocksInfo(variable, step);
}

template <class T>
std::vector<size_t> Engine::GetAbsoluteSteps(const Variable<T> &variable) const
{
    const auto &m = variable.m_AvailableStepBlockIndexOffsets;
    std::vector<size_t> keys;
    keys.reserve(m.size());
    for (auto it = m.cbegin(); it != m.cend(); ++it)
    {
        keys.push_back(it->first - 1);
    }
    return keys;
}

#define declare_type(T, L)                                                     \
    template <>                                                                \
    T *Engine::BufferData(const int bufferIdx, const size_t payloadPosition,   \
                          const size_t bufferID) noexcept                      \
    {                                                                          \
        return DoBufferData_##L(bufferIdx, payloadPosition, bufferID);         \
    }
ADIOS2_FOREACH_PRIMITVE_STDTYPE_2ARGS(declare_type)
#undef declare_type

// PROTECTED
template <class T>
Variable<T> &Engine::FindVariable(const std::string &variableName,
                                  const std::string hint)
{
    Variable<T> *variable = m_IO.InquireVariable<T>(variableName);
    if (variable == nullptr)
    {
        throw std::invalid_argument("ERROR: variable " + variableName +
                                    " not found in IO " + m_IO.m_Name + ", " +
                                    hint + "\n");
    }
    return *variable;
}

// PRIVATE
template <class T>
void Engine::CommonChecks(Variable<T> &variable, const T *data,
                          const std::set<Mode> &modes,
                          const std::string hint) const
{
    variable.CheckDimensions(hint);
    CheckOpenModes(modes, " for variable " + variable.m_Name + ", " + hint);

    // If no dimension has a zero count then there must be data to write.
    if (std::find(variable.m_Count.begin(), variable.m_Count.end(), 0) ==
        variable.m_Count.end())
    {
        helper::CheckForNullptr(
            data, "for data argument in non-zero count block, " + hint);
    }
}

} // end namespace core
} // end namespace adios2

#endif /** ADIOS2_CORE_ENGINE_TCC_ */
