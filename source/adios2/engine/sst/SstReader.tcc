/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SstReader.tcc
 *
 *  Created on: May 1, 2018
 *      Author: Jason Wang
 */

#ifndef ADIOS2_ENGINE_SST_SST_READER_TCC_
#define ADIOS2_ENGINE_SST_SST_READER_TCC_

#include "SstReader.h"

#include "adios2/ADIOSMPI.h"
#include "adios2/helper/adiosFunctions.h" //GetType<T>

namespace adios2
{
namespace core
{
namespace engine
{

template <class T>
void SstReader::ReadVariableBlocks(Variable<T> &variable)
{
    std::vector<void *> sstReadHandlers;
    std::vector<NonContiguousBpBuffer> nonContiguousBpBuffer;

    for (typename Variable<T>::Info &blockInfo : variable.m_BlocksInfo)
    {
        T *originalBlockData = blockInfo.Data;
        for (const auto &stepPair : blockInfo.StepBlockSubStreamsInfo)
        {
            const std::vector<helper::SubStreamBoxInfo> &subStreamsInfo =
                stepPair.second;
            for (const helper::SubStreamBoxInfo &subStreamInfo : subStreamsInfo)
            {
                const size_t rank = subStreamInfo.SubStreamID;
                void *dp_info = NULL;
                if (m_CurrentStepMetaData->DP_TimestepInfo)
                {
                    dp_info = m_CurrentStepMetaData->DP_TimestepInfo[rank];
                }
                // if remote data buffer is compressed
                if (subStreamInfo.OperationsInfo.size() > 0)
                {
                    const bool identity =
                        m_BP3Deserializer->IdentityOperation<T>(
                            blockInfo.Operations);
                    const helper::BlockOperationInfo &blockOperationInfo =
                        m_BP3Deserializer->InitPostOperatorBlockData(
                            subStreamInfo.OperationsInfo,
                            variable.m_RawMemory[1], identity);
                    // if identity is true, just read the entire block content
                    char *output =
                        identity ? reinterpret_cast<char *>(blockInfo.Data)
                                 : variable.m_RawMemory[1].data();
                    auto ret = SstReadRemoteMemory(
                        m_Input, rank, CurrentStep(),
                        blockOperationInfo.PayloadOffset,
                        blockOperationInfo.PayloadSize, output, dp_info);
                    SstWaitForCompletion(m_Input, ret);
                    if (identity)
                    {
                        continue;
                    }
                    m_BP3Deserializer->GetPreOperatorBlockData(
                        variable.m_RawMemory[1], blockOperationInfo,
                        variable.m_RawMemory[0]);
                    m_BP3Deserializer->ClipContiguousMemory<T>(
                        blockInfo, variable.m_RawMemory[0],
                        subStreamInfo.BlockBox, subStreamInfo.IntersectionBox);
                }
                // if remote data buffer is not compressed
                else
                {
                    const auto &seeks = subStreamInfo.Seeks;
                    const size_t writerBlockStart = seeks.first;
                    const size_t writerBlockSize = seeks.second - seeks.first;
                    size_t elementOffset, dummy;
                    // if both input and output are contiguous memory then
                    // directly issue SstRead and put data in place
                    if (helper::IsIntersectionContiguousSubarray(
                            subStreamInfo.BlockBox,
                            subStreamInfo.IntersectionBox,
                            m_BP3Deserializer->m_IsRowMajor, dummy) &&
                        helper::IsIntersectionContiguousSubarray(
                            helper::StartEndBox(
                                blockInfo.Start, blockInfo.Count,
                                m_BP3Deserializer->m_ReverseDimensions),
                            subStreamInfo.IntersectionBox,
                            m_BP3Deserializer->m_IsRowMajor, elementOffset))
                    {
                        auto ret = SstReadRemoteMemory(
                            m_Input, rank, CurrentStep(), writerBlockStart,
                            writerBlockSize, blockInfo.Data + elementOffset,
                            dp_info);
                        sstReadHandlers.push_back(ret);
                    }
                    // if either input or output is not contiguous memory then
                    // find all contiguous parts.
                    else
                    {
                        // if configured to buffer all sst read requests then
                        // put all read requests in a vector
                        if (m_BufferNonContiguousVariables)
                        {
                            nonContiguousBpBuffer.emplace_back();
                            nonContiguousBpBuffer.back().VariableName =
                                variable.m_Name;
                            nonContiguousBpBuffer.back()
                                .ContiguousMemory.resize(writerBlockSize);
                            nonContiguousBpBuffer.back().BlockBox =
                                subStreamInfo.BlockBox;
                            nonContiguousBpBuffer.back().IntersectionBox =
                                subStreamInfo.IntersectionBox;
                            auto ret = SstReadRemoteMemory(
                                m_Input, rank, CurrentStep(), writerBlockStart,
                                writerBlockSize, nonContiguousBpBuffer.back()
                                                     .ContiguousMemory.data(),
                                dp_info);
                            sstReadHandlers.push_back(ret);
                        }
                        // if not configured to buffer all sst read requests
                        // then immediately issue each SstRead. This is NOT
                        // recommended as blocking read will harm performance.
                        else
                        {
                            std::vector<char> contiguousMemory(writerBlockSize);
                            auto ret = SstReadRemoteMemory(
                                m_Input, rank, CurrentStep(), writerBlockStart,
                                writerBlockSize, contiguousMemory.data(),
                                dp_info);
                            SstWaitForCompletion(m_Input, ret);
                            m_BP3Deserializer->ClipContiguousMemory<T>(
                                blockInfo, contiguousMemory,
                                subStreamInfo.BlockBox,
                                subStreamInfo.IntersectionBox);
                        }
                    }
                }
            }
            // advance pointer to next step
            blockInfo.Data += helper::GetTotalSize(blockInfo.Count);
        }
        // move back to original position
        blockInfo.Data = originalBlockData;
    }

    // wait for all SstRead requests to finish
    for (const auto &i : sstReadHandlers)
    {
        SstWaitForCompletion(m_Input, i);
    }

    size_t blockID = 0;

    for (const auto &i : nonContiguousBpBuffer)
    {
        m_BP3Deserializer->ClipContiguousMemory<T>(
            variable.m_BlocksInfo.at(blockID), i.ContiguousMemory, i.BlockBox,
            i.IntersectionBox);
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_SST_SST_READER_TCC_ */
