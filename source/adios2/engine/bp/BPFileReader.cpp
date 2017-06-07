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

#include "adios2/helper/adiosFunctions.h" // CSVToVector

namespace adios
{

BPFileReader::BPFileReader(IO &io, const std::string &name,
                           const OpenMode openMode, MPI_Comm mpiComm)
: Engine("BPFileReader", io, name, openMode, mpiComm)
{
    Init();
}

void BPFileReader::Close(const int /*transportIndex*/) {}

// PRIVATE
void BPFileReader::Init()
{
    if (m_DebugMode)
    {
        if (m_OpenMode != OpenMode::Read)
        {
            throw std::invalid_argument(
                "ERROR: BPFileReader only supports OpenMode::r from" + m_Name +
                " " + m_EndMessage);
        }
    }

    InitTransports();
}

void BPFileReader::InitTransports() {}

VariableBase *BPFileReader::InquireVariableUnknown(const std::string & /*name*/,
                                                   const bool /*readIn*/)
{
    // not yet implemented
    return nullptr;
}

Variable<char> *BPFileReader::InquireVariableChar(const std::string &name,
                                                  const bool readIn)
{
    return InquireVariableCommon<char>(name, readIn);
}

Variable<unsigned char> *
BPFileReader::InquireVariableUChar(const std::string &name, const bool readIn)
{
    return InquireVariableCommon<unsigned char>(name, readIn);
}

Variable<short> *BPFileReader::InquireVariableShort(const std::string &name,
                                                    const bool readIn)
{
    return InquireVariableCommon<short>(name, readIn);
}

Variable<unsigned short> *
BPFileReader::InquireVariableUShort(const std::string &name, const bool readIn)
{
    return InquireVariableCommon<unsigned short>(name, readIn);
}

Variable<int> *BPFileReader::InquireVariableInt(const std::string &name,
                                                const bool readIn)
{
    return InquireVariableCommon<int>(name, readIn);
}

Variable<unsigned int> *
BPFileReader::InquireVariableUInt(const std::string &name, const bool readIn)
{
    return InquireVariableCommon<unsigned int>(name, readIn);
}

Variable<long int> *BPFileReader::InquireVariableLInt(const std::string &name,
                                                      const bool readIn)
{
    return InquireVariableCommon<long int>(name, readIn);
}

Variable<unsigned long int> *
BPFileReader::InquireVariableULInt(const std::string &name, const bool readIn)
{
    return InquireVariableCommon<unsigned long int>(name, readIn);
}

Variable<long long int> *
BPFileReader::InquireVariableLLInt(const std::string &name, const bool readIn)
{
    return InquireVariableCommon<long long int>(name, readIn);
}

Variable<unsigned long long int> *
BPFileReader::InquireVariableULLInt(const std::string &name, const bool readIn)
{
    return InquireVariableCommon<unsigned long long int>(name, readIn);
}

Variable<float> *BPFileReader::InquireVariableFloat(const std::string &name,
                                                    const bool readIn)
{
    return InquireVariableCommon<float>(name, readIn);
}

Variable<double> *BPFileReader::InquireVariableDouble(const std::string &name,
                                                      const bool readIn)
{
    return InquireVariableCommon<double>(name, readIn);
}

Variable<long double> *
BPFileReader::InquireVariableLDouble(const std::string &name, const bool readIn)
{
    return InquireVariableCommon<long double>(name, readIn);
}

Variable<std::complex<float>> *
BPFileReader::InquireVariableCFloat(const std::string &name, const bool readIn)
{
    return InquireVariableCommon<std::complex<float>>(name, readIn);
}

Variable<std::complex<double>> *
BPFileReader::InquireVariableCDouble(const std::string &name, const bool readIn)
{
    return InquireVariableCommon<std::complex<double>>(name, readIn);
}

Variable<std::complex<long double>> *
BPFileReader::InquireVariableCLDouble(const std::string &name,
                                      const bool readIn)
{
    return InquireVariableCommon<std::complex<long double>>(name, readIn);
}

VariableCompound *
BPFileReader::InquireVariableCompound(const std::string & /*name*/,
                                      const bool /*readIn*/)
{
    return nullptr;
}

} // end namespace adios
