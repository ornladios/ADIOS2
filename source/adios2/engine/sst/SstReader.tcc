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
template <class T>
void SstReader::SstBPPerformGets()
{

    const auto &readScheduleMap =
        m_BP3Deserializer->PerformGetsVariablesSubFileInfo(m_IO);
    const auto &variableMap = m_IO.GetAvailableVariables();
    std::vector<void *> sstReadHandlers;
    std::vector<NonContiguousBpBuffer> nonContiguousBpBuffer;
    for (const auto &readSchedule : readScheduleMap)
    {
        const std::string variableName(readSchedule.first);
        for (const auto &subFileIndexPair : readSchedule.second)
        {
            for (const auto &stepPair : subFileIndexPair.second)
            {
                const std::vector<SubFileInfo> &sfis = stepPair.second;
                for (const auto &sfi : sfis)
                {
                    const auto it = variableMap.find(variableName);
                    if (it == variableMap.end())
                    {
                        throw std::runtime_error("SstReader::PerformGets() "
                                                 "failed to find "
                                                 "variable.");
                    }
                    std::string type = "null";
                    for (const auto &parameter : it->second)
                    {
                        if (parameter.first == "Type")
                        {
                            type = parameter.second;
                        }
                    }
                    if (type == "compound")
                    {
                        throw("Compound type is not supported yet.");
                    }
                    else if (type == GetType<T>())
                    {
                        auto *v = m_IO.InquireVariable<T>(variableName);
                        if (v != nullptr)
                        {
                            const size_t rank = subFileIndexPair.first;
                            const auto &seek = sfi.Seeks;
                            const size_t writerBlockStart = seek.first;
                            const size_t writerBlockSize =
                                seek.second - seek.first;
                            size_t elementOffset, dummy;
                            void *dp_info = NULL;
                            if (m_CurrentStepMetaData->DP_TimestepInfo)
                            {
                                dp_info = m_CurrentStepMetaData
                                              ->DP_TimestepInfo[rank];
                            }
                            if (IsIntersectionContiguousSubarray(
                                    sfi.BlockBox, sfi.IntersectionBox,
                                    m_BP3Deserializer->m_IsRowMajor, dummy) &&
                                IsIntersectionContiguousSubarray(
                                    StartEndBox(
                                        v->m_Start, v->m_Count,
                                        m_BP3Deserializer->m_ReverseDimensions),
                                    sfi.IntersectionBox,
                                    m_BP3Deserializer->m_IsRowMajor,
                                    elementOffset))
                            {
                                auto ret = SstReadRemoteMemory(
                                    m_Input, rank, CurrentStep(),
                                    writerBlockStart, writerBlockSize,
                                    v->GetData() + elementOffset, dp_info);
                                sstReadHandlers.push_back(ret);
                            }
                            else
                            {
                                if (m_BufferNonContiguousVariables)
                                {
                                    nonContiguousBpBuffer.emplace_back();
                                    nonContiguousBpBuffer.back().VariableName =
                                        variableName;
                                    nonContiguousBpBuffer.back()
                                        .ContiguousMemory.resize(
                                            writerBlockSize);
                                    nonContiguousBpBuffer.back().BlockBox =
                                        sfi.BlockBox;
                                    nonContiguousBpBuffer.back()
                                        .IntersectionBox = sfi.IntersectionBox;
                                    auto ret = SstReadRemoteMemory(
                                        m_Input, rank, CurrentStep(),
                                        writerBlockStart, writerBlockSize,
                                        nonContiguousBpBuffer.back()
                                            .ContiguousMemory.data(),
                                        dp_info);
                                    sstReadHandlers.push_back(ret);
                                }
                                else
                                {
                                    std::vector<char> contiguousMemory(
                                        writerBlockSize);
                                    auto ret = SstReadRemoteMemory(
                                        m_Input, rank, CurrentStep(),
                                        writerBlockStart, writerBlockSize,
                                        contiguousMemory.data(), dp_info);
                                    SstWaitForCompletion(m_Input, ret);
                                    m_BP3Deserializer->ClipContiguousMemory(
                                        variableName, m_IO, contiguousMemory,
                                        sfi.BlockBox, sfi.IntersectionBox);
                                }
                            }
                        }
                        else
                        {
                            throw std::runtime_error(
                                "In SstReader::PerformGets() data pointer "
                                "obtained from BP "
                                "deserializer is a nullptr");
                        }
                    }
                }
            }
        }
    }
    for (const auto &i : sstReadHandlers)
    {
        SstWaitForCompletion(m_Input, i);
    }
    for (const auto &i : nonContiguousBpBuffer)
    {
        m_BP3Deserializer->ClipContiguousMemory(i.VariableName, m_IO,
                                                i.ContiguousMemory, i.BlockBox,
                                                i.IntersectionBox);
    }
}

} // end namespace adios2

#endif /* ADIOS2_ENGINE_SST_SST_READER_TCC_ */
