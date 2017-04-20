/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS1Reader.cpp
 *
 *  Created on: Feb 27, 2017
 *      Author: wfg
 */

#include "ADIOS1Reader.h"

#include "adios2/core/Support.h"
#include "adios2/core/adiosFunctions.h"           // CSVToVector
#include "adios2/transport/file/FStream.h"        // uses C++ fstream
#include "adios2/transport/file/FileDescriptor.h" // uses POSIX
#include "adios2/transport/file/FilePointer.h"    // uses C FILE*

namespace adios
{

ADIOS1Reader::ADIOS1Reader(ADIOS &adios, const std::string &name,
                           const std::string accessMode, MPI_Comm mpiComm,
                           const Method &method)
: Engine(adios, "ADIOS1Reader", name, accessMode, mpiComm, method,
         " ADIOS1Reader constructor (or call to ADIOS Open).\n")
{
    Init();
    adios_read_init_method(m_ReadMethod, mpiComm, "");
    m_fh = adios_read_open(name.c_str(), m_ReadMethod, mpiComm,
                           ADIOS_LOCKMODE_CURRENT, 0.0);
}

ADIOS1Reader::~ADIOS1Reader()
{
    if (m_fh != nullptr)
        adios_read_close(m_fh);
    adios_read_finalize_method(m_ReadMethod);
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

void ADIOS1Reader::ScheduleReadCommon(const std::string &name,
                                      const Dims &ldims, const Dims &offs,
                                      void *data)
{

    uint64_t start[32], count[32];
    for (int i = 0; i < ldims.size(); i++)
    {
        start[i] = (uint64_t)offs[i];
        count[i] = (uint64_t)ldims[i];
    }
    ADIOS_SELECTION *sel =
        adios_selection_boundingbox(ldims.size(), start, count);
    adios_schedule_read(m_fh, sel, name.c_str(), 1, 0, data);
    adios_selection_delete(sel);
}

void ADIOS1Reader::ScheduleRead(Variable<double> &variable, double *values)
{
    ScheduleReadCommon(variable.m_Name, variable.m_LocalDimensions,
                       variable.m_Offsets, (void *)values);
}

void ADIOS1Reader::ScheduleRead(const std::string variableName, double *values)
{
    ScheduleRead(m_ADIOS.GetVariable<double>(variableName), values);
}

void ADIOS1Reader::PerformReads(PerformReadMode mode)
{
    adios_perform_reads(m_fh, (int)mode);
}

void ADIOS1Reader::Close(const int transportIndex) { adios_read_close(m_fh); }

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
            m_ReadMethod = ADIOS_READ_METHOD_BP;
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
