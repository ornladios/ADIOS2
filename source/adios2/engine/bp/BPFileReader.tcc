/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BPFileReader.tcc
 *
 *  Created on: Feb 27, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_ENGINE_BP_BPFILEREADER_TCC_
#define ADIOS2_ENGINE_BP_BPFILEREADER_TCC_

#include "BPFileReader.h"

#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace core
{
namespace engine
{

template <>
inline void BPFileReader::GetSyncCommon(Variable<std::string> &variable,
                                        std::string *data)
{
    m_BP3Deserializer.GetValueFromMetadata(variable, data);
}

template <class T>
inline void BPFileReader::GetSyncCommon(Variable<T> &variable, T *data)
{
    if (variable.m_SingleValue)
    {
        m_BP3Deserializer.GetValueFromMetadata(variable, data);
        return;
    }

    typename Variable<T>::Info &blockInfo =
        m_BP3Deserializer.InitVariableBlockInfo(variable, data);
    m_BP3Deserializer.SetVariableBlockInfo(variable, blockInfo);
    ReadVariableBlocks(variable);
    variable.m_BlocksInfo.clear();
}

template <class T>
void BPFileReader::GetDeferredCommon(Variable<T> &variable, T *data)
{
    // cheap
    if (variable.m_SingleValue)
    {
        m_BP3Deserializer.GetValueFromMetadata(variable, data);
        return;
    }

    // returns immediately without populating data
    m_BP3Deserializer.InitVariableBlockInfo(variable, data);
    m_BP3Deserializer.m_DeferredVariables.insert(variable.m_Name);
}

template <class T>
void BPFileReader::ReadVariableBlocks(Variable<T> &variable)
{
    const bool profile = m_BP3Deserializer.m_Profiler.IsActive;

    for (typename Variable<T>::Info &blockInfo : variable.m_BlocksInfo)
    {
        T *originalBlockData = blockInfo.Data;

        for (const auto &stepPair : blockInfo.StepBlockSubStreamsInfo)
        {
            const std::vector<helper::SubStreamBoxInfo> &subStreamsInfo =
                stepPair.second;

            for (const helper::SubStreamBoxInfo &subStreamInfo : subStreamsInfo)
            {
                const size_t subFileIndex = subStreamInfo.SubStreamID;

                if (m_SubFileManager.m_Transports.count(subFileIndex) == 0)
                {
                    const std::string subFile(
                        m_BP3Deserializer.GetBPSubFileName(m_Name,
                                                           subFileIndex));

                    m_SubFileManager.OpenFileID(
                        subFile, subFileIndex, Mode::Read,
                        {{"transport", "File"}}, profile);
                }

                const Box<size_t> &seeks = subStreamInfo.Seeks;
                const size_t blockStart = seeks.first;
                const size_t blockSize = seeks.second - seeks.first;

                variable.m_Memory.resize(blockSize);
                m_SubFileManager.ReadFile(variable.m_Memory.data(), blockSize,
                                          blockStart, subFileIndex);

                m_BP3Deserializer.ClipContiguousMemory<T>(
                    blockInfo, variable.m_Memory, subStreamInfo.BlockBox,
                    subStreamInfo.IntersectionBox);
            }
            // advance pointer to next step
            blockInfo.Data += helper::GetTotalSize(blockInfo.Count);
        }
        blockInfo.Data = originalBlockData;
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_BP_BPFILEREADER_TCC_ */
