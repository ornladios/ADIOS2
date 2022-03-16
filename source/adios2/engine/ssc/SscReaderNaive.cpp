/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SscReaderNaive.cpp
 *
 *  Created on: Mar 7, 2022
 *      Author: Jason Wang
 */

#include "SscReaderNaive.tcc"

namespace adios2
{
namespace core
{
namespace engine
{
namespace ssc
{

SscReaderNaive::SscReaderNaive(IO &io, const std::string &name, const Mode mode,
                               MPI_Comm comm)
: SscReaderBase(io, name, mode, comm)
{
}

StepStatus SscReaderNaive::BeginStep(const StepMode stepMode,
                                     const float timeoutSeconds,
                                     const bool readerLocked)
{

    m_Buffer.clear();
    m_BlockMap.clear();

    ++m_CurrentStep;

    size_t globalSize;

    if (m_ReaderRank == 0)
    {
        MPI_Recv(&globalSize, 1, MPI_UNSIGNED_LONG_LONG,
                 m_WriterMasterStreamRank, 0, m_StreamComm, MPI_STATUS_IGNORE);
        m_Buffer.resize(globalSize);
        MPI_Recv(m_Buffer.data(), globalSize, MPI_UNSIGNED_LONG_LONG,
                 m_WriterMasterStreamRank, 0, m_StreamComm, MPI_STATUS_IGNORE);
    }

    MPI_Bcast(&globalSize, 1, MPI_UNSIGNED_LONG_LONG, 0, m_ReaderComm);
    if (m_ReaderRank != 0)
    {
        m_Buffer.resize(globalSize);
    }
    if (globalSize == 1)
    {
        return StepStatus::EndOfStream;
    }
    MPI_Bcast(m_Buffer.data(), globalSize, MPI_CHAR, 0, m_ReaderComm);

    uint64_t pos = 0;
    while (pos < m_Buffer.size())
    {
        uint64_t start = pos;
        uint64_t end = pos + m_Buffer.value<uint64_t>(pos);
        pos += m_Buffer.value<uint64_t>(pos + 8);

        while (pos < end)
        {
            uint8_t shapeId = m_Buffer[pos];
            ++pos;

            if (shapeId == 66)
            {
                DeserializeAttribute(m_Buffer, pos, m_IO, true);
            }
            else
            {
                pos += 4;
                ssc::BlockInfo b;
                DeserializeVariable(m_Buffer, static_cast<ShapeID>(shapeId),
                                    pos, b, m_IO, true);
                b.bufferStart += start;
                m_BlockMap[b.name].push_back(b);
            }
        }
    }

    return StepStatus::OK;
}

size_t SscReaderNaive::CurrentStep() { return m_CurrentStep; }

void SscReaderNaive::EndStep(const bool readerLocked) {}

void SscReaderNaive::PerformGets() {}

void SscReaderNaive::Close(const int transportIndex) {}

#define declare_type(T)                                                        \
    void SscReaderNaive::GetDeferred(Variable<T> &variable, T *data)           \
    {                                                                          \
        helper::Log("Engine", "SSCReader", "GetDeferred", variable.m_Name, 0,  \
                    m_ReaderRank, 5, m_Verbosity, helper::LogMode::INFO);      \
        GetDeferredCommon(variable, data);                                     \
    }                                                                          \
    std::vector<typename Variable<T>::BPInfo> SscReaderNaive::BlocksInfo(      \
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
