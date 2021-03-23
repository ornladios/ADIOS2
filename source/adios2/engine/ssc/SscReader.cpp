/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SscReader.cpp
 *
 *  Created on: Nov 1, 2018
 *      Author: Jason Wang
 */

#include "SscReader.tcc"
#include "adios2/helper/adiosComm.h"
#include "adios2/helper/adiosCommMPI.h"
#include "adios2/helper/adiosFunctions.h"
#include "adios2/helper/adiosMpiHandshake.h"

namespace adios2
{
namespace core
{
namespace engine
{

SscReader::SscReader(IO &io, const std::string &name, const Mode mode,
                     helper::Comm comm)
: Engine("SscReader", io, name, mode, std::move(comm))
{
    TAU_SCOPED_TIMER_FUNC();

    helper::GetParameter(m_IO.m_Parameters, "MpiMode", m_MpiMode);
    helper::GetParameter(m_IO.m_Parameters, "Verbose", m_Verbosity);
    helper::GetParameter(m_IO.m_Parameters, "Threading", m_Threading);
    helper::GetParameter(m_IO.m_Parameters, "OpenTimeoutSecs",
                         m_OpenTimeoutSecs);

    SyncMpiPattern();
}

SscReader::~SscReader() { TAU_SCOPED_TIMER_FUNC(); }

void SscReader::BeginStepConsequentFixed()
{
    if (m_MpiMode == "twosided")
    {
        MPI_Waitall(static_cast<int>(m_MpiRequests.size()),
                    m_MpiRequests.data(), MPI_STATUS_IGNORE);
        m_MpiRequests.clear();
    }
    else if (m_MpiMode == "onesidedfencepush")
    {
        MPI_Win_fence(0, m_MpiWin);
    }
    else if (m_MpiMode == "onesidedpostpush")
    {
        MPI_Win_wait(m_MpiWin);
    }
    else if (m_MpiMode == "onesidedfencepull")
    {
        MPI_Win_fence(0, m_MpiWin);
    }
    else if (m_MpiMode == "onesidedpostpull")
    {
        MPI_Win_complete(m_MpiWin);
    }
}

void SscReader::BeginStepFlexible(StepStatus &status)
{
    m_AllReceivingWriterRanks.clear();
    m_Buffer.resize(1, 0);
    m_GlobalWritePattern.clear();
    m_GlobalWritePattern.resize(m_StreamSize);
    m_LocalReadPattern.clear();
    m_GlobalWritePatternJson.clear();
    bool finalStep = SyncWritePattern();
    if (finalStep)
    {
        status = StepStatus::EndOfStream;
        return;
    }
    MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, m_StreamComm, &m_MpiWin);
}

StepStatus SscReader::BeginStep(const StepMode stepMode,
                                const float timeoutSeconds)
{
    TAU_SCOPED_TIMER_FUNC();

    ++m_CurrentStep;

    m_StepBegun = true;

    if (m_Verbosity >= 5)
    {
        std::cout << "SscReader::BeginStep, World Rank " << m_StreamRank
                  << ", Reader Rank " << m_ReaderRank << ", Step "
                  << m_CurrentStep << std::endl;
    }

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
                if (v.type == DataType::None)
                {
                    throw(std::runtime_error("unknown data type"));
                }
                else if (v.type == DataType::String)
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
            std::memcpy(&variable->m_Min, value.data(), value.size());         \
            std::memcpy(&variable->m_Max, value.data(), value.size());         \
            std::memcpy(&variable->m_Value, value.data(), value.size());       \
        }                                                                      \
    }
                ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
                else { throw(std::runtime_error("unknown data type")); }
            }
        }
    }

    if (m_Buffer[0] == 1)
    {
        return StepStatus::EndOfStream;
    }

    return StepStatus::OK;
}

void SscReader::PerformGets()
{

    if (m_CurrentStep == 0 || m_WriterDefinitionsLocked == false ||
        m_ReaderSelectionsLocked == false)
    {
        ssc::JsonToBlockVecVec(m_GlobalWritePatternJson, m_GlobalWritePattern,
                               m_IO, false, false);
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
            helper::NdCopy<T>(m_Buffer.data() + b.bufferStart, b.start,        \
                              b.count, true, true,                             \
                              reinterpret_cast<char *>(br.data), br.start,     \
                              br.count, true, true);                           \
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
                        else { throw(std::runtime_error("unknown data type")); }
                    }
                }
            }
            br.performed = true;
        }
    }
}

size_t SscReader::CurrentStep() const { return m_CurrentStep; }

void SscReader::EndStepFixed()
{
    if (m_CurrentStep == 0)
    {
        MPI_Win_free(&m_MpiWin);
        SyncReadPattern();
        MPI_Win_create(m_Buffer.data(), m_Buffer.size(), 1, MPI_INFO_NULL,
                       m_StreamComm, &m_MpiWin);
    }
    if (m_MpiMode == "twosided")
    {
        for (const auto &i : m_AllReceivingWriterRanks)
        {
            m_MpiRequests.emplace_back();
            MPI_Irecv(m_Buffer.data() + i.second.first,
                      static_cast<int>(i.second.second), MPI_CHAR, i.first, 0,
                      m_StreamComm, &m_MpiRequests.back());
        }
    }
    else if (m_MpiMode == "onesidedfencepush")
    {
        MPI_Win_fence(0, m_MpiWin);
    }
    else if (m_MpiMode == "onesidedpostpush")
    {
        MPI_Win_post(m_WriterGroup, 0, m_MpiWin);
    }
    else if (m_MpiMode == "onesidedfencepull")
    {
        MPI_Win_fence(0, m_MpiWin);
        for (const auto &i : m_AllReceivingWriterRanks)
        {
            MPI_Get(m_Buffer.data() + i.second.first,
                    static_cast<int>(i.second.second), MPI_CHAR, i.first, 0,
                    static_cast<int>(i.second.second), MPI_CHAR, m_MpiWin);
        }
    }
    else if (m_MpiMode == "onesidedpostpull")
    {
        MPI_Win_start(m_WriterGroup, 0, m_MpiWin);
        for (const auto &i : m_AllReceivingWriterRanks)
        {
            MPI_Get(m_Buffer.data() + i.second.first,
                    static_cast<int>(i.second.second), MPI_CHAR, i.first, 0,
                    static_cast<int>(i.second.second), MPI_CHAR, m_MpiWin);
        }
    }
}

void SscReader::EndStepFirstFlexible()
{
    MPI_Win_free(&m_MpiWin);
    SyncReadPattern();
}

void SscReader::EndStepConsequentFlexible() { MPI_Win_free(&m_MpiWin); }

void SscReader::EndBeginStepFirstFlexible()
{
    EndStepFirstFlexible();
    BeginStepFlexible(m_StepStatus);
}

void SscReader::EndBeginStepConsequentFlexible()
{
    EndStepConsequentFlexible();
    BeginStepFlexible(m_StepStatus);
}

void SscReader::EndStep()
{
    TAU_SCOPED_TIMER_FUNC();

    if (m_Verbosity >= 5)
    {
        std::cout << "SscReader::EndStep, World Rank " << m_StreamRank
                  << ", Reader Rank " << m_ReaderRank << ", Step "
                  << m_CurrentStep << std::endl;
    }

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
                    std::thread(&SscReader::EndBeginStepFirstFlexible, this);
            }
            else
            {
                EndStepFirstFlexible();
            }
        }
        else
        {
            if (m_Threading)
            {
                m_EndStepThread = std::thread(
                    &SscReader::EndBeginStepConsequentFlexible, this);
            }
            else
            {
                EndStepConsequentFlexible();
            }
        }
    }

    m_StepBegun = false;
}

void SscReader::SyncMpiPattern()
{
    TAU_SCOPED_TIMER_FUNC();

    MPI_Group streamGroup;
    MPI_Group readerGroup;
    MPI_Comm writerComm;

    helper::HandshakeComm(m_Name, 'r', m_OpenTimeoutSecs, CommAsMPI(m_Comm),
                          streamGroup, m_WriterGroup, readerGroup, m_StreamComm,
                          writerComm, m_ReaderComm);

    m_ReaderRank = m_Comm.Rank();
    m_ReaderSize = m_Comm.Size();
    MPI_Comm_rank(m_StreamComm, &m_StreamRank);
    MPI_Comm_size(m_StreamComm, &m_StreamSize);

    int writerMasterStreamRank = -1;
    MPI_Allreduce(&writerMasterStreamRank, &m_WriterMasterStreamRank, 1,
                  MPI_INT, MPI_MAX, m_StreamComm);

    int readerMasterStreamRank = -1;
    if (m_ReaderRank == 0)
    {
        readerMasterStreamRank = m_StreamRank;
    }
    MPI_Allreduce(&readerMasterStreamRank, &m_ReaderMasterStreamRank, 1,
                  MPI_INT, MPI_MAX, m_StreamComm);
}

bool SscReader::SyncWritePattern()
{
    TAU_SCOPED_TIMER_FUNC();
    if (m_Verbosity >= 5)
    {
        std::cout << "SscReader::SyncWritePattern, World Rank " << m_StreamRank
                  << ", Reader Rank " << m_ReaderRank << ", Step "
                  << m_CurrentStep << std::endl;
    }

    ssc::BroadcastMetadata(m_GlobalWritePatternJson, m_WriterMasterStreamRank,
                           m_StreamComm);

    if (m_ReaderRank == 0)
    {
        std::cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! "
                  << m_GlobalWritePatternJson.capacity() << std::endl;
        for (size_t i = 0; i < m_GlobalWritePatternJson.capacity(); ++i)
        {
            std::cout << i << " : "
                      << static_cast<int>(m_GlobalWritePatternJson[i]) << " : "
                      << *reinterpret_cast<uint64_t *>(
                             m_GlobalWritePatternJson.data() + i)
                      << " : " << m_GlobalWritePatternJson[i] << std::endl;
        }
        std::cout << std::endl;
        std::cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! " << std::endl;
    }

    if (m_GlobalWritePatternJson[0] == 1)
    {
        return true;
    }

    m_WriterDefinitionsLocked = m_GlobalWritePatternJson[1];

    ssc::JsonToBlockVecVec(m_GlobalWritePatternJson, m_GlobalWritePattern, m_IO,
                           true, true);

    if (m_Verbosity >= 20 && m_ReaderRank == 0)
    {
        ssc::PrintBlockVecVec(m_GlobalWritePattern, "Global Write Pattern");
    }
    return false;
}

void SscReader::SyncReadPattern()
{
    TAU_SCOPED_TIMER_FUNC();

    if (m_Verbosity >= 5)
    {
        std::cout << "SscReader::SyncReadPattern, World Rank " << m_StreamRank
                  << ", Reader Rank " << m_ReaderRank << ", Step "
                  << m_CurrentStep << std::endl;
    }

    ssc::Buffer localBuffer(8);
    *localBuffer.data<uint64_t>() = 0;

    ssc::BlockVecToJson(m_LocalReadPattern, localBuffer, m_StreamRank);

    ssc::Buffer globalBuffer;

    ssc::AggregateMetadata(localBuffer, globalBuffer, m_ReaderComm, false,
                           m_ReaderSelectionsLocked);

    ssc::BroadcastMetadata(globalBuffer, m_ReaderMasterStreamRank,
                           m_StreamComm);

    ssc::JsonToBlockVecVec(m_GlobalWritePatternJson, m_GlobalWritePattern, m_IO,
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
            m_Comm.Barrier();
            if (i == m_ReaderRank)
            {
                ssc::PrintBlockVec(m_LocalReadPattern,
                                   "\n\nGlobal Read Pattern on Rank " +
                                       std::to_string(m_ReaderRank));
            }
        }
        m_Comm.Barrier();
    }
}

void SscReader::CalculatePosition(ssc::BlockVecVec &bvv,
                                  ssc::RankPosMap &allRanks)
{
    TAU_SCOPED_TIMER_FUNC();

    size_t bufferPosition = 0;

    for (int rank = 0; rank < static_cast<int>(bvv.size()); ++rank)
    {
        bool hasOverlap = false;
        for (const auto r : allRanks)
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

#define declare_type(T)                                                        \
    void SscReader::DoGetSync(Variable<T> &variable, T *data)                  \
    {                                                                          \
        GetDeferredCommon(variable, data);                                     \
        PerformGets();                                                         \
    }                                                                          \
    void SscReader::DoGetDeferred(Variable<T> &variable, T *data)              \
    {                                                                          \
        GetDeferredCommon(variable, data);                                     \
    }                                                                          \
    std::vector<typename Variable<T>::Info> SscReader::DoBlocksInfo(           \
        const Variable<T> &variable, const size_t step) const                  \
    {                                                                          \
        return BlocksInfoCommon(variable, step);                               \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void SscReader::DoClose(const int transportIndex)
{
    TAU_SCOPED_TIMER_FUNC();
    if (m_Verbosity >= 5)
    {
        std::cout << "SscReader::DoClose, World Rank " << m_StreamRank
                  << ", Reader Rank " << m_ReaderRank << std::endl;
    }

    if (!m_StepBegun)
    {
        BeginStep();
    }

    if (m_WriterDefinitionsLocked && m_ReaderSelectionsLocked)
    {
        MPI_Win_free(&m_MpiWin);
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
