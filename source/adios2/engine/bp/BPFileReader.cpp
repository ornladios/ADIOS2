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

namespace adios2
{

BPFileReader::BPFileReader(IO &io, const std::string &name, const Mode openMode,
                           MPI_Comm mpiComm)
: Engine("BPFileReader", io, name, openMode, mpiComm),
  m_BP1Reader(mpiComm, m_DebugMode), m_FileManager(mpiComm, m_DebugMode)
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
    // for now just use the name
    //    const std::string
    //    metadataFile(m_BP1Reader.GetBPMetadataFileName(m_Name));
    //    m_FileManager.OpenFileTransport(metadataFile, adios2::Mode::Read,
    //    Params(),
    //                                    true);
}

void BPFileReader::InitBuffers()
{
    // here read indices
    // pg index
    // variables index
    // attributes index
}

#define declare(T, L)                                                          \
    Variable<T> *BPFileReader::DoInquireVariable##L(                           \
        const std::string &variableName)                                       \
    {                                                                          \
        return InquireVariableCommon<T>(variableName);                         \
    }
ADIOS2_FOREACH_TYPE_2ARGS(declare)
#undef declare

} // end namespace adios2
