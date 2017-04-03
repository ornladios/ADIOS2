/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS1Reader.cpp
 *
 *  Created on: Feb 27, 2017
 *      Author: wfg
 */

#include "engine/adios1/ADIOS1Reader.h"
#include "ADIOS.h"

extern int adios_verbose_level;
extern int adios_errno;

namespace adios
{

ADIOS1Reader::ADIOS1Reader(ADIOS &adios, const std::string name,
                           const std::string accessMode, MPI_Comm mpiComm,
                           const Method &method, const IOMode iomode,
                           const float timeout_sec, const bool debugMode,
                           const unsigned int nthreads)
: Engine(adios, "ADIOS1Reader", name, accessMode, mpiComm, method,
        debugMode, nthreads,
        " ADIOS1Reader constructor (or call to ADIOS Open).\n")
{
  Init();
  adios_read_init_method(read_method, m_MPIComm, "");
}

ADIOS1Reader::~ADIOS1Reader()
{
    adios_read_finalize_method(read_method);
}

Variable<void> *
ADIOS1Reader::InquireVariable(const std::string name,
                              const bool readIn) // not yet implemented
{
  return nullptr;
}

Variable<char> *ADIOS1Reader::InquireVariableChar(const std::string name,
                                                  const bool readIn)
{
  return InquireVariableCommon<char>(name, readIn);
}

Variable<unsigned char> *
ADIOS1Reader::InquireVariableUChar(const std::string name, const bool readIn)
{
  return InquireVariableCommon<unsigned char>(name, readIn);
}

Variable<short> *ADIOS1Reader::InquireVariableShort(const std::string name,
                                                    const bool readIn)
{
  return InquireVariableCommon<short>(name, readIn);
}

Variable<unsigned short> *
ADIOS1Reader::InquireVariableUShort(const std::string name, const bool readIn)
{
  return InquireVariableCommon<unsigned short>(name, readIn);
}

Variable<int> *ADIOS1Reader::InquireVariableInt(const std::string name,
                                                const bool readIn)
{
  return InquireVariableCommon<int>(name, readIn);
}

Variable<unsigned int> *
ADIOS1Reader::InquireVariableUInt(const std::string name, const bool readIn)
{
  return InquireVariableCommon<unsigned int>(name, readIn);
}

Variable<long int> *ADIOS1Reader::InquireVariableLInt(const std::string name,
                                                      const bool readIn)
{
  return InquireVariableCommon<long int>(name, readIn);
}

Variable<unsigned long int> *
ADIOS1Reader::InquireVariableULInt(const std::string name, const bool readIn)
{
  return InquireVariableCommon<unsigned long int>(name, readIn);
}

Variable<long long int> *
ADIOS1Reader::InquireVariableLLInt(const std::string name, const bool readIn)
{
  return InquireVariableCommon<long long int>(name, readIn);
}

Variable<unsigned long long int> *
ADIOS1Reader::InquireVariableULLInt(const std::string name, const bool readIn)
{
  return InquireVariableCommon<unsigned long long int>(name, readIn);
}

Variable<float> *ADIOS1Reader::InquireVariableFloat(const std::string name,
                                                    const bool readIn)
{
  return InquireVariableCommon<float>(name, readIn);
}

Variable<double> *ADIOS1Reader::InquireVariableDouble(const std::string name,
                                                      const bool readIn)
{
  return InquireVariableCommon<double>(name, readIn);
}

Variable<long double> *
ADIOS1Reader::InquireVariableLDouble(const std::string name, const bool readIn)
{
  return InquireVariableCommon<long double>(name, readIn);
}

Variable<std::complex<float>> *
ADIOS1Reader::InquireVariableCFloat(const std::string name, const bool readIn)
{
  return InquireVariableCommon<std::complex<float>>(name, readIn);
}

Variable<std::complex<double>> *
ADIOS1Reader::InquireVariableCDouble(const std::string name, const bool readIn)
{
  return InquireVariableCommon<std::complex<double>>(name, readIn);
}

Variable<std::complex<long double>> *
ADIOS1Reader::InquireVariableCLDouble(const std::string name, const bool readIn)
{
  return InquireVariableCommon<std::complex<long double>>(name, readIn);
}

VariableCompound *ADIOS1Reader::InquireVariableCompound(const std::string name,
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
          "ERROR: ADIOS1Reader doesn't support access mode " + m_AccessMode +
          ", in call to ADIOS Open or ADIOS1Reader constructor\n");
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
        throw std::invalid_argument("ERROR: transport " + itTransport->second +
                                    " (you mean File?) not supported, in " +
                                    m_Name + m_EndMessage);
    }
  }
}

} // end namespace
