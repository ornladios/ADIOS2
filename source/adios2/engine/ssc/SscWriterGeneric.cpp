/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SscWriterGeneric.cpp
 *
 *  Created on: Mar 3, 2022
 *      Author: Jason Wang
 */

#include "SscWriterGeneric.tcc"

namespace adios2
{
namespace core
{
namespace engine
{
namespace ssc
{

SscWriterGeneric::SscWriterGeneric(IO &io, const std::string &name,
                                   const Mode mode, MPI_Comm comm)
: SscWriterBase(io, name, mode, comm)
{
}

StepStatus SscWriterGeneric::BeginStep(const StepMode mode,
                                       const float timeoutSeconds,
                                       const bool writerLocked)
{

    m_WriterDefinitionsLocked = writerLocked;

    if (m_Threading && m_EndStepThread.joinable())
    {
        m_EndStepThread.join();
    }

    ++m_CurrentStep;

    if (m_CurrentStep == 0 || m_WriterDefinitionsLocked == false ||
        m_ReaderSelectionsLocked == false)
    {
        m_Buffer.resize(1);
        m_Buffer[0] = 0;
        m_GlobalWritePattern.clear();
        m_GlobalWritePattern.resize(m_StreamSize);
        m_GlobalReadPattern.clear();
        m_GlobalReadPattern.resize(m_StreamSize);
    }

    if (m_CurrentStep > 1)
    {
        if (m_WriterDefinitionsLocked && m_ReaderSelectionsLocked)
        {
            MPI_Waitall(static_cast<int>(m_MpiRequests.size()),
                        m_MpiRequests.data(), MPI_STATUSES_IGNORE);
            m_MpiRequests.clear();
        }
        else
        {
            MPI_Win_free(&m_MpiWin);
        }
    }

    return StepStatus::OK;
}

size_t SscWriterGeneric::CurrentStep() { return m_CurrentStep; }

void SscWriterGeneric::PerformPuts() {}

void SscWriterGeneric::EndStep(const bool writerLocked)
{

    m_WriterDefinitionsLocked = writerLocked;

    if (m_CurrentStep == 0)
    {
        if (m_Threading)
        {
            m_EndStepThread =
                std::thread(&SscWriterGeneric::EndStepFirst, this);
        }
        else
        {
            EndStepFirst();
        }
    }
    else
    {
        if (m_WriterDefinitionsLocked && m_ReaderSelectionsLocked)
        {
            EndStepConsequentFixed();
        }
        else
        {
            if (m_Threading)
            {
                m_EndStepThread = std::thread(
                    &SscWriterGeneric::EndStepConsequentFlexible, this);
            }
            else
            {
                EndStepConsequentFlexible();
            }
        }
    }
}

void SscWriterGeneric::Close(const int transportIndex)
{
    if (m_Threading && m_EndStepThread.joinable())
    {
        m_EndStepThread.join();
    }

    if (m_WriterDefinitionsLocked && m_ReaderSelectionsLocked)
    {
        if (m_CurrentStep > 0)
        {
            MPI_Waitall(static_cast<int>(m_MpiRequests.size()),
                        m_MpiRequests.data(), MPI_STATUSES_IGNORE);
            m_MpiRequests.clear();
        }

        m_Buffer[0] = 1;

        std::vector<MPI_Request> requests;
        for (const auto &i : m_AllSendingReaderRanks)
        {
            requests.emplace_back();
            MPI_Isend(m_Buffer.data(), 1, MPI_CHAR, i.first, 0, m_StreamComm,
                      &requests.back());
        }
        MPI_Waitall(static_cast<int>(requests.size()), requests.data(),
                    MPI_STATUS_IGNORE);
    }
    else
    {
        MPI_Win_free(&m_MpiWin);
        SyncWritePattern(true);
    }
}

#define declare_type(T)                                                        \
    void SscWriterGeneric::PutDeferred(Variable<T> &variable, const T *data)   \
    {                                                                          \
        PutDeferredCommon(variable, data);                                     \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void SscWriterGeneric::PutDeferred(VariableStruct &variable, const void *data)
{
}

void SscWriterGeneric::EndStepFirst()
{
    SyncWritePattern();
    MPI_Win_create(m_Buffer.data(), m_Buffer.size(), 1, MPI_INFO_NULL,
                   m_StreamComm, &m_MpiWin);
    MPI_Win_free(&m_MpiWin);
    SyncReadPattern();
}

void SscWriterGeneric::EndStepConsequentFixed()
{
    for (const auto &i : m_AllSendingReaderRanks)
    {
        m_MpiRequests.emplace_back();
        MPI_Isend(m_Buffer.data(), static_cast<int>(m_Buffer.size()), MPI_CHAR,
                  i.first, 0, m_StreamComm, &m_MpiRequests.back());
    }
}

void SscWriterGeneric::EndStepConsequentFlexible()
{
    SyncWritePattern();
    MPI_Win_create(m_Buffer.data(), m_Buffer.size(), 1, MPI_INFO_NULL,
                   m_StreamComm, &m_MpiWin);
}

void SscWriterGeneric::SyncWritePattern(bool finalStep)
{

    helper::Log("Engine", "SscWriter", "SyncWritePattern", "",
                m_Verbosity >= 10 ? m_WriterRank : 0, m_WriterRank, 5,
                m_Verbosity, helper::LogMode::INFO);

    ssc::Buffer localBuffer(8);
    localBuffer.value<uint64_t>() = 8;

    ssc::SerializeVariables(m_GlobalWritePattern[m_StreamRank], localBuffer,
                            m_StreamRank);

    if (m_WriterRank == 0)
    {
        ssc::SerializeAttributes(m_IO, localBuffer);
    }

    ssc::Buffer globalBuffer;

    ssc::AggregateMetadata(localBuffer, globalBuffer, m_WriterComm, finalStep,
                           m_WriterDefinitionsLocked);

    ssc::BroadcastMetadata(globalBuffer, m_WriterMasterStreamRank,
                           m_StreamComm);

    ssc::Deserialize(globalBuffer, m_GlobalWritePattern, m_IO, false, false);

    if (m_Verbosity >= 20 && m_WriterRank == 0)
    {
        ssc::PrintBlockVecVec(m_GlobalWritePattern, "Global Write Pattern");
    }
}

void SscWriterGeneric::SyncReadPattern()
{

    helper::Log("Engine", "SscWriter", "SyncReadPattern", "",
                m_Verbosity >= 10 ? m_WriterRank : 0, m_WriterRank, 5,
                m_Verbosity, helper::LogMode::INFO);

    ssc::Buffer globalBuffer;

    ssc::BroadcastMetadata(globalBuffer, m_ReaderMasterStreamRank,
                           m_StreamComm);

    m_ReaderSelectionsLocked = globalBuffer[1];

    ssc::Deserialize(globalBuffer, m_GlobalReadPattern, m_IO, false, false);
    m_AllSendingReaderRanks = ssc::CalculateOverlap(
        m_GlobalReadPattern, m_GlobalWritePattern[m_StreamRank]);
    CalculatePosition(m_GlobalWritePattern, m_GlobalReadPattern, m_WriterRank,
                      m_AllSendingReaderRanks);

    if (m_Verbosity >= 10)
    {
        for (int i = 0; i < m_WriterSize; ++i)
        {
            MPI_Barrier(m_WriterComm);
            if (i == m_WriterRank)
            {
                ssc::PrintRankPosMap(m_AllSendingReaderRanks,
                                     "Rank Pos Map for Writer " +
                                         std::to_string(m_WriterRank));
            }
        }
        MPI_Barrier(m_WriterComm);
    }
}

void SscWriterGeneric::CalculatePosition(ssc::BlockVecVec &writerVecVec,
                                         ssc::BlockVecVec &readerVecVec,
                                         const int writerRank,
                                         ssc::RankPosMap &allOverlapRanks)
{
    for (auto &overlapRank : allOverlapRanks)
    {
        auto &readerRankMap = readerVecVec[overlapRank.first];
        auto currentReaderOverlapWriterRanks =
            CalculateOverlap(writerVecVec, readerRankMap);
        size_t bufferPosition = 0;
        for (int rank = 0; rank < static_cast<int>(writerVecVec.size()); ++rank)
        {
            bool hasOverlap = false;
            for (const auto &r : currentReaderOverlapWriterRanks)
            {
                if (r.first == rank)
                {
                    hasOverlap = true;
                    break;
                }
            }
            if (hasOverlap)
            {
                currentReaderOverlapWriterRanks[rank].first = bufferPosition;
                auto &bv = writerVecVec[rank];
                size_t currentRankTotalSize = TotalDataSize(bv) + 1;
                currentReaderOverlapWriterRanks[rank].second =
                    currentRankTotalSize;
                bufferPosition += currentRankTotalSize;
            }
        }
        allOverlapRanks[overlapRank.first] =
            currentReaderOverlapWriterRanks[writerRank];
    }
}

}
}
}
}
