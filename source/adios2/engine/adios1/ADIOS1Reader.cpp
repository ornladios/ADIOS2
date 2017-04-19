/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS1Reader.cpp
 *
 *  Created on: Feb 27, 2017
 *      Author: wfg
 */

#include "core/Support.h"
#include "core/adiosFunctions.h"           // CSVToVector
#include "transport/file/FStream.h"        // uses C++ fstream
#include "transport/file/FileDescriptor.h" // uses POSIX
#include "transport/file/FilePointer.h"    // uses C FILE*

#include "ADIOS1Reader.h"

namespace adios
{

ADIOS1Reader::ADIOS1Reader(ADIOS &adios, const std::string &name,
                           const std::string accessMode, MPI_Comm mpiComm,
                           const Method &method)
: Engine(adios, "ADIOS1Reader", name, accessMode, mpiComm, method,
         " ADIOS1Reader constructor (or call to ADIOS Open).\n")
{
    Init();
    adios_read_init_method(read_method, mpiComm, "");
}

Variable<void> *
ADIOS1Reader::InquireVariable(const std::string &variableName,
                              const bool readIn) // not yet implemented
{
    return nullptr;
}

Variable<char> *
ADIOS1Reader::InquireVariableChar(const std::string &variableName,
                                  const bool readIn)
{
    return InquireVariableCommon<char>(variableName, readIn);
}

Variable<unsigned char> *
ADIOS1Reader::InquireVariableUChar(const std::string &variableName,
                                   const bool readIn)
{
    return InquireVariableCommon<unsigned char>(variableName, readIn);
}

Variable<short> *
ADIOS1Reader::InquireVariableShort(const std::string &variableName,
                                   const bool readIn)
{
    return InquireVariableCommon<short>(variableName, readIn);
}

Variable<unsigned short> *
ADIOS1Reader::InquireVariableUShort(const std::string &variableName,
                                    const bool readIn)
{
    return InquireVariableCommon<unsigned short>(variableName, readIn);
}

Variable<int> *ADIOS1Reader::InquireVariableInt(const std::string &variableName,
                                                const bool readIn)
{
    return InquireVariableCommon<int>(variableName, readIn);
}

Variable<unsigned int> *
ADIOS1Reader::InquireVariableUInt(const std::string &variableName,
                                  const bool readIn)
{
    return InquireVariableCommon<unsigned int>(variableName, readIn);
}

Variable<long int> *
ADIOS1Reader::InquireVariableLInt(const std::string &variableName,
                                  const bool readIn)
{
    return InquireVariableCommon<long int>(variableName, readIn);
}

Variable<unsigned long int> *
ADIOS1Reader::InquireVariableULInt(const std::string &variableName,
                                   const bool readIn)
{
    return InquireVariableCommon<unsigned long int>(variableName, readIn);
}

Variable<long long int> *
ADIOS1Reader::InquireVariableLLInt(const std::string &variableName,
                                   const bool readIn)
{
    return InquireVariableCommon<long long int>(variableName, readIn);
}

Variable<unsigned long long int> *
ADIOS1Reader::InquireVariableULLInt(const std::string &variableName,
                                    const bool readIn)
{
    return InquireVariableCommon<unsigned long long int>(variableName, readIn);
}

Variable<float> *
ADIOS1Reader::InquireVariableFloat(const std::string &variableName,
                                   const bool readIn)
{
    return InquireVariableCommon<float>(variableName, readIn);
}

Variable<double> *
ADIOS1Reader::InquireVariableDouble(const std::string &variableName,
                                    const bool readIn)
{
    return InquireVariableCommon<double>(variableName, readIn);
}

Variable<long double> *
ADIOS1Reader::InquireVariableLDouble(const std::string &variableName,
                                     const bool readIn)
{
    return InquireVariableCommon<long double>(variableName, readIn);
}

Variable<std::complex<float>> *
ADIOS1Reader::InquireVariableCFloat(const std::string &variableName,
                                    const bool readIn)
{
    return InquireVariableCommon<std::complex<float>>(variableName, readIn);
}

Variable<std::complex<double>> *
ADIOS1Reader::InquireVariableCDouble(const std::string &variableName,
                                     const bool readIn)
{
    return InquireVariableCommon<std::complex<double>>(variableName, readIn);
}

Variable<std::complex<long double>> *
ADIOS1Reader::InquireVariableCLDouble(const std::string &variableName,
                                      const bool readIn)
{
    return InquireVariableCommon<std::complex<long double>>(variableName,
                                                            readIn);
}

VariableCompound *
ADIOS1Reader::InquireVariableCompound(const std::string &variableName,
                                      const bool readIn)
{
    return nullptr;
}

void ADIOS1Reader::Close(const int transportIndex) {}

// PRIVATE
void ADIOS1Reader::Init()
{
    if (m_DebugMode == true)
    {
        if (m_AccessMode != "r" && m_AccessMode != "read")
            throw std::invalid_argument(
                "ERROR: ADIOS1Reader doesn't support access mode " +
                m_AccessMode +
                ", in call to ADIOS Open or ADIOS1Reader constructor\n");
    }
    InitParameters();
    InitTransports();
}

void ADIOS1Reader::InitParameters() {}

void ADIOS1Reader::InitTransports()
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
        if (itTransport->second == "file" || itTransport->second == "File" ||
            itTransport->second == "bp" || itTransport->second == "BP")
        {
            read_method = ADIOS_READ_METHOD_BP;
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
