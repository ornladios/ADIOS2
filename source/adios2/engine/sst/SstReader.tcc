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
#include "adios2/toolkit/profiling/taustubs/tautimer.hpp"

namespace adios2
{
namespace core
{
namespace engine
{

template <class T>
void SstReader::ReadVariableBlocks(Variable<T> &variable)
{
    TAU_SCOPED_TIMER_FUNC();
    std::vector<void *> sstReadHandlers;
    std::vector<std::vector<char>> buffers;

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
                    // TODO test with compression
                    char *buffer = nullptr;
                    size_t payloadSize = 0, payloadStart = 0;

                    m_BP3Deserializer->PreDataRead(
                        variable, blockInfo, subStreamInfo, buffer, payloadSize,
                        payloadStart, 0);

                    std::stringstream ss;
                    ss << "SST Bytes Read from remote rank " << rank;
                    TAU_SAMPLE_COUNTER(ss.str().c_str(), payloadSize);
                    auto ret = SstReadRemoteMemory(m_Input, rank, CurrentStep(),
                                                   payloadStart, payloadSize,
                                                   buffer, dp_info);
                    sstReadHandlers.push_back(ret);
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
                        // batch all read requests
                        buffers.emplace_back();
                        buffers.back().resize(writerBlockSize);
                        auto ret = SstReadRemoteMemory(
                            m_Input, rank, CurrentStep(), writerBlockStart,
                            writerBlockSize, buffers.back().data(), dp_info);
                        sstReadHandlers.push_back(ret);
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

    size_t iter = 0;

    for (typename Variable<T>::Info &blockInfo : variable.m_BlocksInfo)
    {
        T *originalBlockData = blockInfo.Data;
        for (const auto &stepPair : blockInfo.StepBlockSubStreamsInfo)
        {
            const std::vector<helper::SubStreamBoxInfo> &subStreamsInfo =
                stepPair.second;
            for (const helper::SubStreamBoxInfo &subStreamInfo : subStreamsInfo)
            {
                // if remote data buffer is compressed
                if (subStreamInfo.OperationsInfo.size() > 0)
                {
                    // const bool identity =
                    //    m_BP3Deserializer->IdentityOperation<T>(
                    //        blockInfo.Operations);
                    // const helper::BlockOperationInfo
                    //    &blockOperationInfo =
                    //        m_BP3Deserializer->InitPostOperatorBlockData(
                    //            subStreamInfo.OperationsInfo,
                    //            variable.m_RawMemory[1],
                    //            identity);
                    // m_BP3Deserializer->GetPreOperatorBlockData(
                    //    buffers[iter], blockOperationInfo,
                    //    variable.m_RawMemory[0]);
                    // helper::ClipVector(variable.m_RawMemory[0],
                    //    subStreamInfo.Seeks.first,
                    //    subStreamInfo.Seeks.second);
                    // m_BP3Deserializer->ClipContiguousMemory<T>(
                    //    blockInfo,
                    //    variable.m_RawMemory[0],
                    //    subStreamInfo.BlockBox,
                    //    subStreamInfo.IntersectionBox);

                    m_BP3Deserializer->PostDataRead(
                        variable, blockInfo, subStreamInfo,
                        helper::IsRowMajor(m_IO.m_HostLanguage), 0);
                    ++iter;
                }
                // if remote data buffer is not compressed
                else
                {
                    size_t dummy;
                    // if both input and output are contiguous memory then
                    // directly issue SstRead and put data in place
                    if (helper::IsIntersectionContiguousSubarray(
                            subStreamInfo.BlockBox,
                            subStreamInfo.IntersectionBox,
                            m_BP3Deserializer->m_IsRowMajor, dummy) == false ||
                        helper::IsIntersectionContiguousSubarray(
                            helper::StartEndBox(
                                blockInfo.Start, blockInfo.Count,
                                m_BP3Deserializer->m_ReverseDimensions),
                            subStreamInfo.IntersectionBox,
                            m_BP3Deserializer->m_IsRowMajor, dummy) == false)
                    {
                        size_t blockID = 0;
                        m_BP3Deserializer->ClipContiguousMemory<T>(
                            variable.m_BlocksInfo.at(blockID), buffers[iter],
                            subStreamInfo.BlockBox,
                            subStreamInfo.IntersectionBox);
                        ++iter;
                    }
                }
            }
            // advance pointer to next step
            blockInfo.Data += helper::GetTotalSize(blockInfo.Count);
        }
        // move back to original position
        blockInfo.Data = originalBlockData;
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_SST_SST_READER_TCC_ */
