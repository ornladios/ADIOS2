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

template <class T>
static std::vector<typename Variable<T>::Info> ToBlocksInfo(
    const std::vector<typename core::Variable<T>::Info> &coreBlocksInfo)
{
    std::vector<typename Variable<T>::Info> blocksInfo;
    blocksInfo.reserve(coreBlocksInfo.size());

    for (const typename core::Variable<T>::Info &coreBlockInfo : coreBlocksInfo)
    {
        typename Variable<T>::Info blockInfo;
        blockInfo.Start = coreBlockInfo.Start;
        blockInfo.Count = coreBlockInfo.Count;
        blockInfo.IsValue = coreBlockInfo.IsValue;
        if (blockInfo.IsValue)
        {
            blockInfo.Value = coreBlockInfo.Value;
        }
        else
        {
            blockInfo.Min = coreBlockInfo.Min;
            blockInfo.Max = coreBlockInfo.Max;
            blockInfo.Data = coreBlockInfo.Buffer;
        }
        blocksInfo.push_back(blockInfo);
    }

    return blocksInfo;
}

template <class T>
void Engine::Put(Variable<T> variable, const T *data, const Mode launch)
{
    adios2::helper::CheckForNullptr(m_Engine, "in call to Engine::Put");
    m_Engine->Put<T>(*variable.m_Variable, data, launch);
}

template <class T>
void Engine::Put(const std::string &variableName, const T *data,
                 const Mode launch)
{
    adios2::helper::CheckForNullptr(m_Engine, "in call to Engine::Put");
    m_Engine->Put<T>(variableName, data, launch);
}

template <class T>
void Engine::Put(Variable<T> variable, const T &datum, const Mode /*launch*/)
{
    adios2::helper::CheckForNullptr(m_Engine, "in call to Engine::Put");
    m_Engine->Put(*variable.m_Variable, datum);
}

template <class T>
void Engine::Put(const std::string &variableName, const T &datum,
                 const Mode /*launch*/)
{
    adios2::helper::CheckForNullptr(m_Engine, "in call to Engine::Put");
    m_Engine->Put<T>(variableName, datum);
}

template <class T>
void Engine::Get(Variable<T> variable, T *data, const Mode launch)
{
    adios2::helper::CheckForNullptr(m_Engine, "in call to Engine::Get");
    m_Engine->Get<T>(*variable.m_Variable, data, launch);
}

template <class T>
void Engine::Get(const std::string &variableName, T *data, const Mode launch)
{
    adios2::helper::CheckForNullptr(m_Engine, "in call to Engine::Get");
    m_Engine->Get<T>(variableName, data, launch);
}

template <class T>
void Engine::Get(Variable<T> variable, T &datum, const Mode /*launch*/)
{
    adios2::helper::CheckForNullptr(m_Engine, "in call to Engine::Get");
    m_Engine->Get<T>(*variable.m_Variable, datum);
}

template <class T>
void Engine::Get(const std::string &variableName, T &datum,
                 const Mode /*launch*/)
{
    adios2::helper::CheckForNullptr(m_Engine, "in call to Engine::Get");
    m_Engine->Get<T>(variableName, datum);
}

template <class T>
void Engine::Get(Variable<T> variable, std::vector<T> &dataV, const Mode launch)
{
    adios2::helper::CheckForNullptr(
        m_Engine, "in call to Engine::Get with std::vector argument");
    m_Engine->Get<T>(*variable.m_Variable, dataV, launch);
}

template <class T>
void Engine::Get(const std::string &variableName, std::vector<T> &dataV,
                 const Mode launch)
{
    adios2::helper::CheckForNullptr(
        m_Engine, "in call to Engine::Get with std::vector argument");
    m_Engine->Get<T>(variableName, dataV, launch);
}

template <class T>
void Engine::Get(Variable<T> variable, const Mode launch)
{
    adios2::helper::CheckForNullptr(m_Engine, "in call to Engine::Get");
    m_Engine->Get<T>(*variable.m_Variable, launch);
}

template <class T>
void Engine::Get(const std::string &variableName, const Mode launch)
{
    adios2::helper::CheckForNullptr(m_Engine, "in call to Engine::Get");
    m_Engine->Get<T>(variableName, launch);
}

template <class T>
std::map<size_t, std::vector<typename Variable<T>::Info>>
Engine::AllStepsBlocksInfo(const Variable<T> variable) const
{
    adios2::helper::CheckForNullptr(
        m_Engine, "for Engine in call to Engine::AllStepsBlocksInfo");
    adios2::helper::CheckForNullptr(
        variable.m_Variable,
        "for variable in call to Engine::AllStepsBlocksInfo");

    const std::map<size_t, std::vector<typename core::Variable<T>::Info>>
        coreAllStepsBlockInfo =
            m_Engine->AllStepsBlocksInfo<T>(*variable.m_Variable);

    std::map<size_t, std::vector<typename Variable<T>::Info>>
        allStepsBlocksInfo;

    for (const auto &pair : coreAllStepsBlockInfo)
    {
        const size_t step = pair.first;
        const std::vector<typename core::Variable<T>::Info> &coreBlocksInfo =
            pair.second;

        allStepsBlocksInfo[step] = ToBlocksInfo<T>(coreBlocksInfo);
    }
    return allStepsBlocksInfo;
}

template <class T>
std::vector<typename Variable<T>::Info>
Engine::BlocksInfo(const Variable<T> variable, const size_t step) const
{
    adios2::helper::CheckForNullptr(m_Engine,
                                    "for Engine in call to Engine::BlocksInfo");
    adios2::helper::CheckForNullptr(
        variable.m_Variable, "for variable in call to Engine::BlocksInfo");

    const auto blocksInfo = m_Engine->BlocksInfo<T>(*variable.m_Variable, step);
    return ToBlocksInfo<T>(blocksInfo);
}

} // end namespace adios2

#endif /* ADIOS2_BINDINGS_CXX11_CXX11_ENGINE_TCC_ */
