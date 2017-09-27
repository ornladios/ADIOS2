/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BPFileReader.cpp
 *
 *  Created on: Feb 27, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "BPFileReader.h"
#include "BPFileReader.tcc"

#include "adios2/helper/adiosFunctions.h" // MPI BroadcastVector

namespace adios2
{

BPFileReader::BPFileReader(IO &io, const std::string &name, const Mode openMode,
                           MPI_Comm mpiComm)
: Engine("BPFileReader", io, name, openMode, mpiComm),
  m_BP1BuffersReader(mpiComm, m_DebugMode), m_FileManager(mpiComm, m_DebugMode)
{
    Init();
}

void BPFileReader::Close(const int /*transportIndex*/) {}

// PRIVATE
void BPFileReader::Init()
{
    if (m_DebugMode)
    {
        if (m_OpenMode != Mode::Read)
        {
            throw std::invalid_argument(
                "ERROR: BPFileReader only supports OpenMode::Read from" +
                m_Name + " " + m_EndMessage);
        }
    }

    InitTransports();
    InitBuffers();
}

void BPFileReader::InitTransports()
{
    if (m_IO.m_TransportsParameters.empty())
    {
        Params defaultTransportParameters;
        defaultTransportParameters["transport"] = "File";
        m_IO.m_TransportsParameters.push_back(defaultTransportParameters);
    }

    if (m_BP1BuffersReader.m_RankMPI == 0)
    {
        const std::string metadataFile(
            m_BP1BuffersReader.GetBPMetadataFileName(m_Name));
        m_FileManager.OpenFiles({}, {metadataFile}, adios2::Mode::Read,
                                m_IO.m_TransportsParameters, true);
    }
}

void BPFileReader::InitBuffers()
{
    // Put all metadata in buffer
    if (m_BP1BuffersReader.m_RankMPI == 0)
    {
        const size_t fileSize = m_FileManager.GetFileSize(0);
        m_BP1BuffersReader.m_Metadata.Resize(
            fileSize,
            "allocating metadata buffer, in call to BPFileReader Open");

        m_FileManager.ReadFile(m_BP1BuffersReader.m_Metadata.m_Buffer.data(),
                               fileSize);
    }
    // broadcast vector to all ranks from zero
    BroadcastVector(m_BP1BuffersReader.m_Metadata.m_Buffer, m_MPIComm);

    // fills IO with Variables and Attributes
    m_BP1BuffersReader.ParseMetadata(m_IO);
}

#define declare(T, L)                                                          \
    Variable<T> *BPFileReader::DoInquireVariable##L(                           \
        const std::string &variableName)                                       \
    {                                                                          \
        return InquireVariableCommon<T>(variableName);                         \
    }
ADIOS2_FOREACH_TYPE_2ARGS(declare)
#undef declare

#define declare_type(T)                                                        \
    void BPFileReader::DoRead(Variable<T> &variable, T *values)                \
    {                                                                          \
        ReadCommon(variable, values);                                          \
    }

ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

} // end namespace adios2
