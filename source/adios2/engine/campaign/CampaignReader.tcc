/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CampaignReader.tcc
 *
 *  Created on: May 15, 2023
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#ifndef ADIOS2_ENGINE_CAMPAIGNREADER_TCC_
#define ADIOS2_ENGINE_CAMPAIGNREADER_TCC_

#include "CampaignReader.h"

#include <adios2-perfstubs-interface.h>

#include <fstream>
#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

template <class T>
inline Variable<T> CampaignReader::DuplicateVariable(Variable<T> *variable, IO &io,
                                                     std::string &name, VarInternalInfo &vii)
{
    auto &v = io.DefineVariable<T>(name, variable->Shape());
    v.m_AvailableStepsCount = variable->GetAvailableStepsCount();
    v.m_AvailableStepsStart = variable->GetAvailableStepsStart();
    v.m_ShapeID = variable->m_ShapeID;
    v.m_SingleValue = variable->m_SingleValue;
    v.m_ReadAsJoined = variable->m_ReadAsJoined;
    v.m_ReadAsLocalValue = variable->m_ReadAsLocalValue;
    v.m_RandomAccess = variable->m_RandomAccess;
    v.m_MemSpace = variable->m_MemSpace;
    v.m_JoinedDimPos = variable->m_JoinedDimPos;
    v.m_AvailableStepBlockIndexOffsets = variable->m_AvailableStepBlockIndexOffsets;
    v.m_AvailableShapes = variable->m_AvailableShapes;
    v.m_Min = variable->m_Min;
    v.m_Max = variable->m_Max;
    v.m_Value = variable->m_Value;
    v.m_StepsStart = variable->m_StepsStart;
    v.m_StepsCount = variable->m_StepsCount;
    v.m_Start = variable->m_Start;
    v.m_Count = variable->m_Count;
    v.m_AccuracyRequested = variable->m_AccuracyRequested;
    v.m_AccuracyProvided = variable->m_AccuracyProvided;

    v.m_Engine = this; // Variable::Shape() uses this member to call engine
    vii.originalVar = static_cast<void *>(variable);
    m_VarInternalInfo.emplace(name, vii);
    return v;
}

template <class T>
inline Attribute<T> CampaignReader::DuplicateAttribute(Attribute<T> *attribute, IO &io,
                                                       std::string &name)
{
    if (attribute->m_IsSingleValue)
    {
        auto &a = io.DefineAttribute<T>(name, attribute->m_DataSingleValue);
        return a;
    }
    auto &a = io.DefineAttribute<T>(name, attribute->m_DataArray.data(), attribute->m_Elements);
    return a;
}

template <class T>
std::pair<Variable<T> *, Engine *> CampaignReader::FindActualVariable(Variable<T> &variable)
{
    auto it = m_VarInternalInfo.find(variable.m_Name);
    if (it == m_VarInternalInfo.end())
    {
        return std::make_pair(nullptr, nullptr);
    }

    Variable<T> *v = reinterpret_cast<Variable<T> *>(it->second.originalVar);
    Engine *e = m_Engines[it->second.engineIdx];

    return std::make_pair(v, e);
}

template <class T>
Variable<T> *CampaignReader::CopyPropertiesToActualVariable(Variable<T> &campaignVariable,
                                                            Variable<T> *actualVariable)
{
    actualVariable->m_SelectionType = campaignVariable.m_SelectionType;
    actualVariable->m_Start = campaignVariable.m_Start;
    actualVariable->m_Count = campaignVariable.m_Count;
    actualVariable->m_StepsStart = campaignVariable.m_StepsStart;
    actualVariable->m_StepsCount = campaignVariable.m_StepsCount;
    actualVariable->m_BlockID = campaignVariable.m_BlockID;
    actualVariable->m_MemoryStart = campaignVariable.m_MemoryStart;
    actualVariable->m_MemoryCount = campaignVariable.m_MemoryCount;
    actualVariable->m_MemSpace = campaignVariable.m_MemSpace;
    actualVariable->m_AccuracyRequested = campaignVariable.m_AccuracyRequested;
    return actualVariable;
}

template <class T>
void CampaignReader::GetSyncTCC(Variable<T> &variable, T *data)
{
    PERFSTUBS_SCOPED_TIMER("CampaignReader::Get");
    std::pair<Variable<T> *, Engine *> p = FindActualVariable(variable);
    if (p.first != nullptr)
    {
        CopyPropertiesToActualVariable(variable, p.first);
        p.second->Get(*p.first, data, adios2::Mode::Sync);
    }
    else if (DataType::Char == helper::GetDataType<T>() ||
             DataType::UInt8 == helper::GetDataType<T>())
    {
        auto it = m_CampaignVarInternalInfo.find(variable.m_Name);
        if (it != m_CampaignVarInternalInfo.end())
        {
            // FIXME: Selection support for TEXT/IMAGE variables
            if (!it->second.path.empty())
            {
                // local file
                std::ifstream is(it->second.path, std::ios::binary);
                is.seekg(0, std::ios::end);
                size_t length = is.tellg();
                is.seekg(0, std::ios::beg);
                if (m_Options.verbose > 1)
                {
                    std::cout << "---- Read local file " << it->second.path << "  size = " << length
                              << std::endl;
                }
                is.read((char *)data, length);
                is.close();
            }
            else if (m_CampaignData.datasets[it->second.dsIdx]
                         .replicas[it->second.repIdx]
                         .files.size())
            {
                // has embedded file
                GetVariableFromDB(it->first, it->second.dsIdx, it->second.repIdx, variable.m_Type,
                                  (void *)data);
            }
            else
            {
                // remote file
                auto &ds = m_CampaignData.datasets[it->second.dsIdx];
                auto &rep = ds.replicas[it->second.repIdx];
                if (m_Options.verbose > 0)
                {
                    std::cout << " -- Read remote file "
                              << m_CampaignData.hosts[rep.hostIdx].hostname << ":"
                              << m_CampaignData.directory[rep.dirIdx].path << PathSeparator
                              << rep.name << "  of size " << rep.size << "\n";
                }
                std::string remotePath =
                    m_CampaignData.directory[rep.dirIdx].path + PathSeparator + rep.name;
                ReadRemoteFile(m_CampaignData.hosts[rep.hostIdx].hostname, remotePath, rep.size,
                               (void *)data);
            }
        }
    }
}

template <class T>
void CampaignReader::GetDeferredTCC(Variable<T> &variable, T *data)
{
    PERFSTUBS_SCOPED_TIMER("CampaignReader::Get");
    std::pair<Variable<T> *, Engine *> p = FindActualVariable(variable);
    if (p.first != nullptr)
    {
        CopyPropertiesToActualVariable(variable, p.first);
        p.second->Get(*p.first, data, adios2::Mode::Deferred);
    }
}

template <class T>
std::map<size_t, std::vector<typename Variable<T>::BPInfo>>
CampaignReader::AllStepsBlocksInfoTCC(const Variable<T> &variable) const
{
}

template <class T>
std::vector<std::vector<typename Variable<T>::BPInfo>>
CampaignReader::AllRelativeStepsBlocksInfoTCC(const Variable<T> &variable) const
{
}

template <class T>
std::vector<typename Variable<T>::BPInfo> CampaignReader::BlocksInfoTCC(const Variable<T> &variable,
                                                                        const size_t step) const
{
}

std::vector<typename core::Variable<std::string>::BPInfo>
CampaignReader::BlocksInfoCommon(const core::Variable<std::string> &variable,
                                 const std::vector<size_t> &blocksIndexOffsets) const
{
    std::vector<typename core::Variable<std::string>::BPInfo> blocksInfo;
    /*
        blocksInfo.reserve(1);

        typename core::Variable<std::string>::BPInfo blockInfo;
        blockInfo.Shape = blockCharacteristics.Shape;
        blockInfo.Start = blockCharacteristics.Start;
        blockInfo.Count = blockCharacteristics.Count;
        blockInfo.WriterID = blockCharacteristics.Statistics.FileIndex;
        blockInfo.IsReverseDims = m_ReverseDimensions;

        if (m_ReverseDimensions)
        {
            std::reverse(blockInfo.Shape.begin(), blockInfo.Shape.end());
            std::reverse(blockInfo.Start.begin(), blockInfo.Start.end());
            std::reverse(blockInfo.Count.begin(), blockInfo.Count.end());
        }

        if (blockCharacteristics.Statistics.IsValue) // value
        {
            blockInfo.IsValue = true;
            blockInfo.Value = blockCharacteristics.Statistics.Value;
        }
        else // array
        {
            blockInfo.IsValue = false;
            blockInfo.Min = blockCharacteristics.Statistics.Min;
            blockInfo.Max = blockCharacteristics.Statistics.Max;
            blockInfo.MinMaxs = blockCharacteristics.Statistics.MinMaxs;
            blockInfo.SubBlockInfo = blockCharacteristics.Statistics.SubBlockInfo;
        }
        if (blockInfo.Shape.size() == 1 && blockInfo.Shape.front() == LocalValueDim)
        {
            blockInfo.Shape = Dims{blocksIndexOffsets.size()};
            blockInfo.Count = Dims{1};
            blockInfo.Start = Dims{n};
            blockInfo.Min = blockCharacteristics.Statistics.Value;
            blockInfo.Max = blockCharacteristics.Statistics.Value;
        }
        // bp index starts at 1
        blockInfo.Step = static_cast<size_t>(blockCharacteristics.Statistics.Step - 1);
        blockInfo.BlockID = n;
        blocksInfo.push_back(blockInfo);
    */
    return blocksInfo;
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif // ADIOS2_ENGINE_CAMPAIGNREADER_TCC_
