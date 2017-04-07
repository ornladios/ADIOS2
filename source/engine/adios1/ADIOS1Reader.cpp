/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BPFileReader.cpp
 *
 *  Created on: Feb 27, 2017
 *      Author: wfg
 */

#include "core/Support.h"
#include "engine/bp/BPFileReader.h"
#include "functions/adiosFunctions.h"      // CSVToVector
#include "transport/file/FStream.h"        // uses C++ fstream
#include "transport/file/FileDescriptor.h" // uses POSIX
#include "transport/file/FilePointer.h"    // uses C FILE*

namespace adios
{

ADIOS1Reader::ADIOS1Reader(ADIOS &adios, const std::string &name,
                           const std::string accessMode, MPI_Comm mpiComm,
                           const Method &method)
: Engine(adios, "BPFileReader", name, accessMode, mpiComm, method,
         " BPFileReader constructor (or call to ADIOS Open).\n")
{
    Init();
}

BPFileReader::~BPFileReader() {}

Variable<void> *
BPFileReader::InquireVariable(const std::string &variableName,
                              const bool readIn) // not yet implemented
{
    return nullptr;
}

Variable<char> *
BPFileReader::InquireVariableChar(const std::string &variableName,
                                  const bool readIn)
{
    return InquireVariableCommon<char>(variableName, readIn);
}

Variable<unsigned char> *
BPFileReader::InquireVariableUChar(const std::string &variableName,
                                   const bool readIn)
{
    return InquireVariableCommon<unsigned char>(variableName, readIn);
}

Variable<short> *
BPFileReader::InquireVariableShort(const std::string &variableName,
                                   const bool readIn)
{
    return InquireVariableCommon<short>(variableName, readIn);
}

Variable<unsigned short> *
BPFileReader::InquireVariableUShort(const std::string &variableName,
                                    const bool readIn)
{
    return InquireVariableCommon<unsigned short>(variableName, readIn);
}

Variable<int> *BPFileReader::InquireVariableInt(const std::string &variableName,
                                                const bool readIn)
{
    return InquireVariableCommon<int>(variableName, readIn);
}

Variable<unsigned int> *
BPFileReader::InquireVariableUInt(const std::string &variableName,
                                  const bool readIn)
{
    return InquireVariableCommon<unsigned int>(variableName, readIn);
}

Variable<long int> *
BPFileReader::InquireVariableLInt(const std::string &variableName,
                                  const bool readIn)
{
    return InquireVariableCommon<long int>(variableName, readIn);
}

Variable<unsigned long int> *
BPFileReader::InquireVariableULInt(const std::string &variableName,
                                   const bool readIn)
{
    return InquireVariableCommon<unsigned long int>(variableName, readIn);
}

Variable<long long int> *
BPFileReader::InquireVariableLLInt(const std::string &variableName,
                                   const bool readIn)
{
    return InquireVariableCommon<long long int>(variableName, readIn);
}

Variable<unsigned long long int> *
BPFileReader::InquireVariableULLInt(const std::string &variableName,
                                    const bool readIn)
{
    return InquireVariableCommon<unsigned long long int>(variableName, readIn);
}

Variable<float> *
BPFileReader::InquireVariableFloat(const std::string &variableName,
                                   const bool readIn)
{
    return InquireVariableCommon<float>(variableName, readIn);
}

Variable<double> *
BPFileReader::InquireVariableDouble(const std::string &variableName,
                                    const bool readIn)
{
    return InquireVariableCommon<double>(variableName, readIn);
}

Variable<long double> *
BPFileReader::InquireVariableLDouble(const std::string &variableName,
                                     const bool readIn)
{
    return InquireVariableCommon<long double>(variableName, readIn);
}

Variable<std::complex<float>> *
BPFileReader::InquireVariableCFloat(const std::string &variableName,
                                    const bool readIn)
{
    return InquireVariableCommon<std::complex<float>>(variableName, readIn);
}

Variable<std::complex<double>> *
BPFileReader::InquireVariableCDouble(const std::string &variableName,
                                     const bool readIn)
{
    return InquireVariableCommon<std::complex<double>>(variableName, readIn);
}

Variable<std::complex<long double>> *
BPFileReader::InquireVariableCLDouble(const std::string &variableName,
                                      const bool readIn)
{
    return InquireVariableCommon<std::complex<long double>>(variableName,
                                                            readIn);
}

VariableCompound *
BPFileReader::InquireVariableCompound(const std::string &variableName,
                                      const bool readIn)
{
    return nullptr;
}

void BPFileReader::Close(const int transportIndex) {}

// PRIVATE
void BPFileReader::Init()
{
    if (m_DebugMode == true)
    {
        if (m_AccessMode != "r" && m_AccessMode != "read")
            throw std::invalid_argument(
                "ERROR: BPFileReader doesn't support access mode " +
                m_AccessMode +
                ", in call to ADIOS Open or BPFileReader constructor\n");
    }

    InitTransports();
}

void BPFileReader::InitTransports() // maybe move this?
{
    if (m_DebugMode == true)
    {
        if (TransportNamesUniqueness() == false)
        {
            throw std::invalid_argument(
                "ERROR: two transports of the same kind (e.g file IO) "
                "can't have the same name, modify with name= in Method "
                "AddTransport\n");
        }
    }

    for (const auto &parameters : m_Method.m_TransportParameters)
    {
        auto itTransport = parameters.find("transport");
        if (itTransport->second == "file" || itTransport->second == "File")
        {
            auto itLibrary = parameters.find("library");
            if (itLibrary == parameters.end() ||
                itLibrary->second == "POSIX") // use default POSIX
            {
                auto file = std::make_shared<transport::FileDescriptor>(
                    m_MPIComm, m_DebugMode);
                // m_BP1Reader.OpenRankFiles( m_Name, m_AccessMode, *file );
                m_Transports.push_back(std::move(file));
            }
            else if (itLibrary->second == "FILE*" ||
                     itLibrary->second == "stdio.h")
            {
                auto file = std::make_shared<transport::FilePointer>(
                    m_MPIComm, m_DebugMode);
                // m_BP1Reader.OpenRankFiles( m_Name, m_AccessMode, *file );
                m_Transports.push_back(std::move(file));
            }
            else if (itLibrary->second == "fstream" ||
                     itLibrary->second == "std::fstream")
            {
                auto file = std::make_shared<transport::FStream>(m_MPIComm,
                                                                 m_DebugMode);
                // m_BP1Reader.OpenRankFiles( m_Name, m_AccessMode, *file );
                m_Transports.push_back(std::move(file));
            }
            else if (itLibrary->second == "MPI-IO")
            {
            }
            else
            {
                if (m_DebugMode == true)
                    throw std::invalid_argument(
                        "ERROR: file transport library " + itLibrary->second +
                        " not supported, in " + m_Name + m_EndMessage);
            }
        }
        else
        {
            if (m_DebugMode == true)
                throw std::invalid_argument(
                    "ERROR: transport " + itTransport->second +
                    " (you mean File?) not supported, in " + m_Name +
                    m_EndMessage);
        }
    }
}

} // end namespace
