/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * InSituMPIWriter.tcc implementation of template functions with known type
 *
 *  Created on: Dec 18, 2017
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */
#ifndef ADIOS2_ENGINE_INSITU_INSITUMPIWRITER_TCC_
#define ADIOS2_ENGINE_INSITU_INSITUMPIWRITER_TCC_

#include "InSituMPIWriter.h"

#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

template <class T>
void InSituMPIWriter::PutSyncCommon(Variable<T> &variable,
                                    const typename Variable<T>::Info &blockInfo)
{
    if (variable.m_SingleValue)
    {
        if (m_Verbosity == 5)
        {
            std::cout << "InSituMPI Writer " << m_WriterRank << " PutSync("
                      << variable.m_Name << ") = " << *blockInfo.Data
                      << std::endl;
        }
        const size_t dataSize = m_BP3Serializer.GetBPIndexSizeInData(
            variable.m_Name, blockInfo.Count);
        format::BP3Base::ResizeResult resizeResult =
            m_BP3Serializer.ResizeBuffer(dataSize, "in call to variable " +
                                                       variable.m_Name +
                                                       " PutSync");

        if (resizeResult == format::BP3Base::ResizeResult::Flush)
        {
            throw std::runtime_error(
                "ERROR: InSituMPI write engine PutDeferred(" + variable.m_Name +
                ") caused Flush which is not handled).");
        }

        // WRITE INDEX to data buffer and metadata structure (in memory) we only
        // need the metadata structure but this is the granularity of
        // the function call
        m_BP3Serializer.PutVariableMetadata(variable, blockInfo);
    }
    else
    {
        throw std::invalid_argument(
            "ERROR: ADIOS InSituMPI engine: PytSync(" + variable.m_Name +
            ") is not supported for arrays, only for single values.\n");
    }
}

template <class T>
void InSituMPIWriter::PutDeferredCommon(Variable<T> &variable, const T *values)
{
    auto &blockInfo = variable.SetBlockInfo(values, m_CurrentStep);

    if (m_Verbosity == 5)
    {
        std::cout << "InSituMPI Writer " << m_WriterRank << " PutDeferred("
                  << variable.m_Name << ")\n";
    }

    const size_t dataSize =
        m_BP3Serializer.GetBPIndexSizeInData(variable.m_Name, variable.m_Count);
    format::BP3Base::ResizeResult resizeResult = m_BP3Serializer.ResizeBuffer(
        dataSize, "in call to variable " + variable.m_Name + " PutDeferred");

    if (resizeResult == format::BP3Base::ResizeResult::Flush)
    {
        throw std::runtime_error("ERROR: InSituMPI write engine PutDeferred(" +
                                 variable.m_Name +
                                 ") caused Flush which is not handled).");
    }

    // WRITE INDEX to data buffer and metadata structure (in memory)
    // we only need the metadata structure but this is the granularity of the
    // function call
    m_BP3Serializer.PutVariableMetadata(variable, blockInfo);

    if (m_WriterDefinitionsLocked && m_RemoteDefinitionsLocked)
    {
        // Create the async send for the variable now
        AsyncSendVariable(variable, blockInfo);
    }
    else
    {
        // Remember this variable to make the send request in PerformPuts()
        m_BP3Serializer.m_DeferredVariables.insert(variable.m_Name);
    }
}

template <class T>
void InSituMPIWriter::AsyncSendVariable(
    Variable<T> &variable, const typename Variable<T>::Info &blockInfo)
{
    const auto it = m_WriteScheduleMap.find(variable.m_Name);
    if (it != m_WriteScheduleMap.end())
    {
        std::map<size_t, std::vector<helper::SubFileInfo>> requests =
            it->second;
        Box<Dims> mybox =
            helper::StartEndBox(variable.m_Start, variable.m_Count);
        for (const auto &readerPair : requests)
        {
            for (const auto &sfi : readerPair.second)
            {
                if (helper::IdenticalBoxes(mybox, sfi.BlockBox))
                {
                    if (m_Verbosity == 5)
                    {
                        std::cout << "InSituMPI Writer " << m_WriterRank
                                  << " async send var = " << variable.m_Name
                                  << " to reader " << readerPair.first
                                  << " block=";
                        insitumpi::PrintBox(mybox);
                        std::cout << " info = ";
                        insitumpi::PrintSubFileInfo(sfi);
                        std::cout << std::endl;
                    }

                    m_MPIRequests.emplace_back();

                    const auto &seek = sfi.Seeks;
                    const size_t blockStart = seek.first;
                    const size_t blockSize = seek.second - seek.first;

                    MPI_Isend(blockInfo.Data + blockStart, blockSize, MPI_CHAR,
                              m_RankAllPeers[readerPair.first],
                              insitumpi::MpiTags::Data, m_CommWorld,
                              &m_MPIRequests.back());
                }
            }
        }
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_INSITUMPI_INSITUWRITER_TCC_ */
