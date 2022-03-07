/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SscReaderGeneric.cpp
 *
 *  Created on: Mar 3, 2022
 *      Author: Jason Wang
 */

#include "SscReaderGeneric.tcc"
#include "adios2/helper/adiosMemory.h"

namespace adios2
{
namespace core
{
namespace engine
{
namespace ssc
{

SscReaderGeneric::SscReaderGeneric(IO &io, const std::string &name,
                                   const Mode mode, MPI_Comm comm)
: SscReaderBase(io, name, mode, comm)
{
}

void SscReaderGeneric::BeginStepConsequentFixed()
{
    MPI_Waitall(static_cast<int>(m_MpiRequests.size()), m_MpiRequests.data(),
                MPI_STATUS_IGNORE);
    m_MpiRequests.clear();
}

void SscReaderGeneric::BeginStepFlexible(StepStatus &status)
{
    m_AllReceivingWriterRanks.clear();
    m_Buffer.resize(1);
    m_Buffer[0] = 0;
    m_GlobalWritePattern.clear();
    m_GlobalWritePattern.resize(m_StreamSize);
    m_LocalReadPattern.clear();
    m_GlobalWritePatternBuffer.clear();
    bool finalStep = SyncWritePattern();
    if (finalStep)
    {
        status = StepStatus::EndOfStream;
        return;
    }
    MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, m_StreamComm, &m_MpiWin);
}

StepStatus SscReaderGeneric::BeginStep(const StepMode stepMode,
                                       const float timeoutSeconds,
                                       const bool readerLocked)
{

    m_ReaderSelectionsLocked = readerLocked;

    ++m_CurrentStep;

    m_StepBegun = true;

    if (m_CurrentStep == 0 || m_WriterDefinitionsLocked == false ||
        m_ReaderSelectionsLocked == false)
    {
        if (m_Threading && m_EndStepThread.joinable())
        {
            m_EndStepThread.join();
        }
        else
        {
            BeginStepFlexible(m_StepStatus);
        }
        if (m_StepStatus == StepStatus::EndOfStream)
        {
            return StepStatus::EndOfStream;
        }
    }
    else
    {
        BeginStepConsequentFixed();
    }

    for (const auto &r : m_GlobalWritePattern)
    {
        for (auto &v : r)
        {
            if (v.shapeId == ShapeID::GlobalValue ||
                v.shapeId == ShapeID::LocalValue)
            {
                std::vector<char> value(v.bufferCount);
                if (m_CurrentStep == 0 || m_WriterDefinitionsLocked == false ||
                    m_ReaderSelectionsLocked == false)
                {
                    std::memcpy(value.data(), v.value.data(), v.value.size());
                }
                else
                {
                    std::memcpy(value.data(), m_Buffer.data() + v.bufferStart,
                                v.bufferCount);
                }

                if (v.type == DataType::String)
                {
                    auto variable = m_IO.InquireVariable<std::string>(v.name);
                    if (variable)
                    {
                        variable->m_Value =
                            std::string(value.begin(), value.end());
                        variable->m_Min =
                            std::string(value.begin(), value.end());
                        variable->m_Max =
                            std::string(value.begin(), value.end());
                    }
                }
#define declare_type(T)                                                        \
    else if (v.type == helper::GetDataType<T>())                               \
    {                                                                          \
        auto variable = m_IO.InquireVariable<T>(v.name);                       \
        if (variable)                                                          \
        {                                                                      \
            std::memcpy(reinterpret_cast<char *>(&variable->m_Min),            \
                        value.data(), value.size());                           \
            std::memcpy(reinterpret_cast<char *>(&variable->m_Max),            \
                        value.data(), value.size());                           \
            std::memcpy(reinterpret_cast<char *>(&variable->m_Value),          \
                        value.data(), value.size());                           \
        }                                                                      \
    }
                ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
                else
                {
                    helper::Log("Engine", "SSCReader", "BeginStep",
                                "unknown data type", 0, m_ReaderRank, 0,
                                m_Verbosity, helper::LogMode::ERROR);
                }
            }
        }
    }

    if (m_Buffer[0] == 1)
    {
        return StepStatus::EndOfStream;
    }

    return StepStatus::OK;
}

size_t SscReaderGeneric::CurrentStep() { return m_CurrentStep; }

void SscReaderGeneric::EndStepFixed()
{
    if (m_CurrentStep == 0)
    {
        MPI_Win_free(&m_MpiWin);
        SyncReadPattern();
    }
    for (const auto &i : m_AllReceivingWriterRanks)
    {
        m_MpiRequests.emplace_back();
        MPI_Irecv(m_Buffer.data() + i.second.first,
                  static_cast<int>(i.second.second), MPI_CHAR, i.first, 0,
                  m_StreamComm, &m_MpiRequests.back());
    }
}

void SscReaderGeneric::EndStepFirstFlexible()
{
    MPI_Win_free(&m_MpiWin);
    SyncReadPattern();
    BeginStepFlexible(m_StepStatus);
}

void SscReaderGeneric::EndStepConsequentFlexible()
{
    MPI_Win_free(&m_MpiWin);
    BeginStepFlexible(m_StepStatus);
}

void SscReaderGeneric::EndStep(const bool readerLocked)
{
    m_ReaderSelectionsLocked = readerLocked;
    PerformGets();

    if (m_WriterDefinitionsLocked && m_ReaderSelectionsLocked)
    {
        EndStepFixed();
    }
    else
    {
        if (m_CurrentStep == 0)
        {
            if (m_Threading)
            {
                m_EndStepThread =
                    std::thread(&SscReaderGeneric::EndStepFirstFlexible, this);
            }
            else
            {
                MPI_Win_free(&m_MpiWin);
                SyncReadPattern();
            }
        }
        else
        {
            if (m_Threading)
            {
                m_EndStepThread = std::thread(
                    &SscReaderGeneric::EndStepConsequentFlexible, this);
            }
            else
            {
                MPI_Win_free(&m_MpiWin);
            }
        }
    }

    m_StepBegun = false;
}

void SscReaderGeneric::PerformGets()
{

    if (m_CurrentStep == 0 || m_WriterDefinitionsLocked == false ||
        m_ReaderSelectionsLocked == false)
    {
        ssc::Deserialize(m_GlobalWritePatternBuffer, m_GlobalWritePattern, m_IO,
                         false, false);
        size_t oldSize = m_AllReceivingWriterRanks.size();
        m_AllReceivingWriterRanks =
            ssc::CalculateOverlap(m_GlobalWritePattern, m_LocalReadPattern);
        CalculatePosition(m_GlobalWritePattern, m_AllReceivingWriterRanks);
        size_t newSize = m_AllReceivingWriterRanks.size();
        if (oldSize != newSize)
        {
            size_t totalDataSize = 0;
            for (auto i : m_AllReceivingWriterRanks)
            {
                totalDataSize += i.second.second;
            }
            m_Buffer.resize(totalDataSize);
            for (const auto &i : m_AllReceivingWriterRanks)
            {
                MPI_Win_lock(MPI_LOCK_SHARED, i.first, 0, m_MpiWin);
                MPI_Get(m_Buffer.data() + i.second.first,
                        static_cast<int>(i.second.second), MPI_CHAR, i.first, 0,
                        static_cast<int>(i.second.second), MPI_CHAR, m_MpiWin);
                MPI_Win_unlock(i.first, m_MpiWin);
            }
        }

        for (auto &br : m_LocalReadPattern)
        {
            if (br.performed)
            {
                continue;
            }
            for (const auto &i : m_AllReceivingWriterRanks)
            {
                const auto &v = m_GlobalWritePattern[i.first];
                for (const auto &b : v)
                {
                    if (b.name == br.name)
                    {
                        if (b.type == DataType::String)
                        {
                            *reinterpret_cast<std::string *>(br.data) =
                                std::string(b.value.begin(), b.value.end());
                        }
#define declare_type(T)                                                        \
    else if (b.type == helper::GetDataType<T>())                               \
    {                                                                          \
        if (b.shapeId == ShapeID::GlobalArray ||                               \
            b.shapeId == ShapeID::LocalArray)                                  \
        {                                                                      \
            bool empty = false;                                                \
            for (const auto c : b.count)                                       \
            {                                                                  \
                if (c == 0)                                                    \
                {                                                              \
                    empty = true;                                              \
                }                                                              \
            }                                                                  \
            if (empty)                                                         \
            {                                                                  \
                continue;                                                      \
            }                                                                  \
            helper::NdCopy(m_Buffer.data<char>() + b.bufferStart, b.start,     \
                           b.count, true, true,                                \
                           reinterpret_cast<char *>(br.data), br.start,        \
                           br.count, true, true, sizeof(T));                   \
        }                                                                      \
        else if (b.shapeId == ShapeID::GlobalValue ||                          \
                 b.shapeId == ShapeID::LocalValue)                             \
        {                                                                      \
            std::memcpy(br.data, m_Buffer.data() + b.bufferStart,              \
                        b.bufferCount);                                        \
        }                                                                      \
    }
                        ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
                        else
                        {
                            helper::Log("Engine", "SSCReader", "PerformGets",
                                        "unknown data type", 0, m_ReaderRank, 0,
                                        m_Verbosity, helper::LogMode::ERROR);
                        }
                    }
                }
            }
            br.performed = true;
        }
    }
}

bool SscReaderGeneric::SyncWritePattern()
{

    ssc::BroadcastMetadata(m_GlobalWritePatternBuffer, m_WriterMasterStreamRank,
                           m_StreamComm);

    if (m_GlobalWritePatternBuffer[0] == 1)
    {
        return true;
    }

    m_WriterDefinitionsLocked = m_GlobalWritePatternBuffer[1];

    ssc::Deserialize(m_GlobalWritePatternBuffer, m_GlobalWritePattern, m_IO,
                     true, true);

    if (m_Verbosity >= 20 && m_ReaderRank == 0)
    {
        ssc::PrintBlockVecVec(m_GlobalWritePattern, "Global Write Pattern");
    }
    return false;
}

void SscReaderGeneric::SyncReadPattern()
{

    ssc::Buffer localBuffer(8);
    localBuffer.value<uint64_t>() = 8;

    ssc::SerializeVariables(m_LocalReadPattern, localBuffer, m_StreamRank);

    ssc::Buffer globalBuffer;

    ssc::AggregateMetadata(localBuffer, globalBuffer, m_ReaderComm, false,
                           m_ReaderSelectionsLocked);

    ssc::BroadcastMetadata(globalBuffer, m_ReaderMasterStreamRank,
                           m_StreamComm);

    ssc::Deserialize(m_GlobalWritePatternBuffer, m_GlobalWritePattern, m_IO,
                     true, true);

    m_AllReceivingWriterRanks =
        ssc::CalculateOverlap(m_GlobalWritePattern, m_LocalReadPattern);
    CalculatePosition(m_GlobalWritePattern, m_AllReceivingWriterRanks);

    size_t totalDataSize = 0;
    for (auto i : m_AllReceivingWriterRanks)
    {
        totalDataSize += i.second.second;
    }
    m_Buffer.resize(totalDataSize);

    if (m_Verbosity >= 20)
    {
        for (int i = 0; i < m_ReaderSize; ++i)
        {
            MPI_Barrier(m_ReaderComm);
            if (i == m_ReaderRank)
            {
                ssc::PrintBlockVec(m_LocalReadPattern,
                                   "\n\nGlobal Read Pattern on Rank " +
                                       std::to_string(m_ReaderRank));
            }
        }
        MPI_Barrier(m_ReaderComm);
    }
}

void SscReaderGeneric::CalculatePosition(ssc::BlockVecVec &bvv,
                                         ssc::RankPosMap &allRanks)
{

    size_t bufferPosition = 0;

    for (int rank = 0; rank < static_cast<int>(bvv.size()); ++rank)
    {
        bool hasOverlap = false;
        for (const auto &r : allRanks)
        {
            if (r.first == rank)
            {
                hasOverlap = true;
                break;
            }
        }
        if (hasOverlap)
        {
            allRanks[rank].first = bufferPosition;
            auto &bv = bvv[rank];
            for (auto &b : bv)
            {
                b.bufferStart += bufferPosition;
            }
            size_t currentRankTotalSize = ssc::TotalDataSize(bv);
            allRanks[rank].second = currentRankTotalSize + 1;
            bufferPosition += currentRankTotalSize + 1;
        }
    }
}

void SscReaderGeneric::Close(const int transportIndex)
{
    if (!m_StepBegun)
    {
        BeginStep(StepMode::Read, -1.0, m_ReaderSelectionsLocked);
    }
}

#define declare_type(T)                                                        \
    void SscReaderGeneric::GetDeferred(Variable<T> &variable, T *data)         \
    {                                                                          \
        helper::Log("Engine", "SSCReader", "GetDeferred", variable.m_Name, 0,  \
                    m_ReaderRank, 5, m_Verbosity, helper::LogMode::INFO);      \
        GetDeferredCommon(variable, data);                                     \
    }                                                                          \
    std::vector<typename Variable<T>::BPInfo> SscReaderGeneric::BlocksInfo(    \
        const Variable<T> &variable, const size_t step) const                  \
    {                                                                          \
        return BlocksInfoCommon(variable, step);                               \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

}
}
}
}
