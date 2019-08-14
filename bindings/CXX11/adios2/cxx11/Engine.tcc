/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Engine.tcc :
 *
 *  Created on: Jun 5, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_CXX11_CXX11_ENGINE_TCC_
#define ADIOS2_BINDINGS_CXX11_CXX11_ENGINE_TCC_

#include "Engine.h"

#include "adios2/core/Engine.h"
#include "adios2/core/Variable.h"
#include "adios2/helper/adiosFunctions.h"

namespace adios2
{

namespace
{
template <class T>
static std::vector<typename Variable<T>::Info>
ToBlocksInfo(const std::vector<typename core::Variable<
                 typename TypeInfo<T>::IOType>::Info> &coreBlocksInfo)
{
    using IOType = typename TypeInfo<T>::IOType;

    std::vector<typename Variable<T>::Info> blocksInfo;
    blocksInfo.reserve(coreBlocksInfo.size());

    for (const typename core::Variable<IOType>::Info &coreBlockInfo :
         coreBlocksInfo)
    {
        typename Variable<T>::Info blockInfo;
        // doesn't work because coreBlockInfo is transient.
        // blockInfo.m_Info = &coreBlockInfo;
        blockInfo.Start = coreBlockInfo.Start;
        blockInfo.Count = coreBlockInfo.Count;
        blockInfo.WriterID = coreBlockInfo.WriterID;

        blockInfo.IsValue = coreBlockInfo.IsValue;
        blockInfo.IsReverseDims = coreBlockInfo.IsReverseDims;
        if (blockInfo.IsValue)
        {
            blockInfo.Value = coreBlockInfo.Value;
        }
        else
        {
            blockInfo.Min = coreBlockInfo.Min;
            blockInfo.Max = coreBlockInfo.Max;
        }
        blockInfo.BlockID = coreBlockInfo.BlockID;
        blocksInfo.push_back(blockInfo);
    }

    return blocksInfo;
}
} // end empty namespace

template <class T>
typename Variable<T>::Span Engine::Put(Variable<T> variable,
                                       const size_t bufferID, const T &value)
{
    using IOType = typename TypeInfo<T>::IOType;
    adios2::helper::CheckForNullptr(m_Engine,
                                    "for Engine in call to Engine::Array");

    if (m_Engine->m_EngineType == "NULL")
    {
        return typename Variable<T>::Span(nullptr);
    }

    adios2::helper::CheckForNullptr(variable.m_Variable,
                                    "for variable in call to Engine::Array");

    typename Variable<T>::Span::CoreSpan *coreSpan =
        reinterpret_cast<typename Variable<T>::Span::CoreSpan *>(
            &m_Engine->Put(*variable.m_Variable, bufferID,
                           reinterpret_cast<const IOType &>(value)));

    return typename Variable<T>::Span(coreSpan);
}

template <class T>
typename Variable<T>::Span Engine::Put(Variable<T> variable)
{
    return Put(variable, 0, T());
}

template <class T>
void Engine::Put(Variable<T> variable, const T *data, const Mode launch)
{
    using IOType = typename TypeInfo<T>::IOType;
    adios2::helper::CheckForNullptr(m_Engine, "in call to Engine::Put");
    if (m_Engine->m_EngineType == "NULL")
    {
        return;
    }
    adios2::helper::CheckForNullptr(variable.m_Variable,
                                    "for variable in call to Engine::Put");
    m_Engine->Put(*variable.m_Variable, reinterpret_cast<const IOType *>(data),
                  launch);
}

template <class T>
void Engine::Put(const std::string &variableName, const T *data,
                 const Mode launch)
{
    using IOType = typename TypeInfo<T>::IOType;
    adios2::helper::CheckForNullptr(m_Engine, "in call to Engine::Put");
    if (m_Engine->m_EngineType == "NULL")
    {
        return;
    }
    m_Engine->Put(variableName, reinterpret_cast<const IOType *>(data), launch);
}

template <class T>
void Engine::Put(Variable<T> variable, const T &datum, const Mode launch)
{
    using IOType = typename TypeInfo<T>::IOType;
    adios2::helper::CheckForNullptr(m_Engine, "in call to Engine::Put");
    if (m_Engine->m_EngineType == "NULL")
    {
        return;
    }
    adios2::helper::CheckForNullptr(variable.m_Variable,
                                    "for variable in call to Engine::Put");
    m_Engine->Put(*variable.m_Variable, reinterpret_cast<const IOType &>(datum),
                  launch);
}

template <class T>
void Engine::Put(const std::string &variableName, const T &datum,
                 const Mode launch)
{
    using IOType = typename TypeInfo<T>::IOType;
    adios2::helper::CheckForNullptr(m_Engine, "in call to Engine::Put");
    if (m_Engine->m_EngineType == "NULL")
    {
        return;
    }
    m_Engine->Put(variableName, reinterpret_cast<const IOType &>(datum),
                  launch);
}

template <class T>
void Engine::Get(Variable<T> variable, T *data, const Mode launch)
{
    using IOType = typename TypeInfo<T>::IOType;
    adios2::helper::CheckForNullptr(m_Engine, "in call to Engine::Get");
    if (m_Engine->m_EngineType == "NULL")
    {
        return;
    }
    adios2::helper::CheckForNullptr(variable.m_Variable,
                                    "for variable in call to Engine::Get");
    m_Engine->Get(*variable.m_Variable, reinterpret_cast<IOType *>(data),
                  launch);
}

template <class T>
void Engine::Get(const std::string &variableName, T *data, const Mode launch)
{
    using IOType = typename TypeInfo<T>::IOType;
    adios2::helper::CheckForNullptr(m_Engine, "in call to Engine::Get");
    if (m_Engine->m_EngineType == "NULL")
    {
        return;
    }
    m_Engine->Get(variableName, reinterpret_cast<IOType *>(data), launch);
}

template <class T>
void Engine::Get(Variable<T> variable, T &datum, const Mode /*launch*/)
{
    using IOType = typename TypeInfo<T>::IOType;
    adios2::helper::CheckForNullptr(m_Engine, "in call to Engine::Get");
    if (m_Engine->m_EngineType == "NULL")
    {
        return;
    }
    adios2::helper::CheckForNullptr(variable.m_Variable,
                                    "for variable in call to Engine::Get");
    m_Engine->Get(*variable.m_Variable, reinterpret_cast<IOType &>(datum));
}

template <class T>
void Engine::Get(const std::string &variableName, T &datum,
                 const Mode /*launch*/)
{
    using IOType = typename TypeInfo<T>::IOType;
    adios2::helper::CheckForNullptr(m_Engine, "in call to Engine::Get");
    if (m_Engine->m_EngineType == "NULL")
    {
        return;
    }
    m_Engine->Get(variableName, reinterpret_cast<IOType &>(datum));
}

template <class T>
void Engine::Get(Variable<T> variable, std::vector<T> &dataV, const Mode launch)
{
    using IOType = typename TypeInfo<T>::IOType;
    adios2::helper::CheckForNullptr(
        m_Engine, "in call to Engine::Get with std::vector argument");
    if (m_Engine->m_EngineType == "NULL")
    {
        return;
    }
    adios2::helper::CheckForNullptr(variable.m_Variable,
                                    "for variable in call to Engine::Get");
    m_Engine->Get(*variable.m_Variable,
                  reinterpret_cast<std::vector<IOType> &>(dataV), launch);
}

template <class T>
void Engine::Get(const std::string &variableName, std::vector<T> &dataV,
                 const Mode launch)
{
    using IOType = typename TypeInfo<T>::IOType;
    adios2::helper::CheckForNullptr(
        m_Engine, "in call to Engine::Get with std::vector argument");
    if (m_Engine->m_EngineType == "NULL")
    {
        return;
    }
    m_Engine->Get(variableName, reinterpret_cast<std::vector<IOType> &>(dataV),
                  launch);
}

template <class T>
void Engine::Get(Variable<T> variable, typename Variable<T>::Info &info,
                 const Mode launch)
{
    using IOType = typename TypeInfo<T>::IOType;
    adios2::helper::CheckForNullptr(m_Engine, "in call to Engine::Get");
    if (m_Engine->m_EngineType == "NULL")
    {
        return;
    }
    adios2::helper::CheckForNullptr(variable.m_Variable,
                                    "for variable in call to Engine::Get");
    info.m_Info = reinterpret_cast<typename Variable<T>::Info::CoreInfo *>(
        m_Engine->Get(*variable.m_Variable, launch));
}

template <class T>
void Engine::Get(const std::string &variableName,
                 typename Variable<T>::Info &info, const Mode launch)
{
    using IOType = typename TypeInfo<T>::IOType;
    adios2::helper::CheckForNullptr(m_Engine, "in call to Engine::Get");
    if (m_Engine->m_EngineType == "NULL")
    {
        return;
    }
    info.m_Info = reinterpret_cast<typename Variable<T>::Info::CoreInfo *>(
        m_Engine->Get<IOType>(variableName, launch));
}

template <class T>
std::map<size_t, std::vector<typename Variable<T>::Info>>
Engine::AllStepsBlocksInfo(const Variable<T> variable) const
{
    using IOType = typename TypeInfo<T>::IOType;
    adios2::helper::CheckForNullptr(
        m_Engine, "for Engine in call to Engine::AllStepsBlocksInfo");
    if (m_Engine->m_EngineType == "NULL")
    {
        return std::map<size_t, std::vector<typename Variable<T>::Info>>();
    }

    adios2::helper::CheckForNullptr(
        variable.m_Variable,
        "for variable in call to Engine::AllStepsBlocksInfo");

    const std::map<size_t, std::vector<typename core::Variable<IOType>::Info>>
        coreAllStepsBlockInfo =
            m_Engine->AllStepsBlocksInfo(*variable.m_Variable);

    std::map<size_t, std::vector<typename Variable<T>::Info>>
        allStepsBlocksInfo;

    for (const auto &pair : coreAllStepsBlockInfo)
    {
        const size_t step = pair.first;
        const std::vector<typename core::Variable<IOType>::Info>
            &coreBlocksInfo = pair.second;

        allStepsBlocksInfo[step] = ToBlocksInfo<T>(coreBlocksInfo);
    }
    return allStepsBlocksInfo;
}

// Design Node: All Info structs are copied. This prevents Engine::Get() from
// connecting the Core Info struct to Binding Info struct when this method is
// called. Instead of returning a vector, BlocksInfo could populate a vector
// member
// of the Variable, and those could contain pointers to the Core Info structs,
// enabling users of the Inline engine to do Info.Data()
template <class T>
std::vector<typename Variable<T>::Info>
Engine::BlocksInfo(const Variable<T> variable, const size_t step) const
{
    using IOType = typename TypeInfo<T>::IOType;
    adios2::helper::CheckForNullptr(m_Engine,
                                    "for Engine in call to Engine::BlocksInfo");
    if (m_Engine->m_EngineType == "NULL")
    {
        return std::vector<typename Variable<T>::Info>();
    }

    adios2::helper::CheckForNullptr(
        variable.m_Variable, "for variable in call to Engine::BlocksInfo");

    const auto blocksInfo =
        m_Engine->BlocksInfo<IOType>(*variable.m_Variable, step);
    return ToBlocksInfo<T>(blocksInfo);
}

} // end namespace adios2

#endif /* ADIOS2_BINDINGS_CXX11_CXX11_ENGINE_TCC_ */
