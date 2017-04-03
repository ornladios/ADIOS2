/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BPFileReader.cpp
 *
 *  Created on: Feb 27, 2017
 *      Author: wfg
 */

#include "engine/bp/BPFileReader.h"

#include "core/Support.h"
#include "functions/adiosFunctions.h"      //CSVToVector
#include "transport/file/FileDescriptor.h" // uses POSIX
#include "transport/file/FilePointer.h"    // uses C FILE*

// supported transports
#include "transport/file/FStream.h" // uses C++ fstream

namespace adios
{

BPFileReader::BPFileReader(ADIOS &adios, std::string name,
                           std::string accessMode, MPI_Comm mpiComm,
                           const Method &method, IOMode /*iomode*/,
                           float /*timeout_sec*/, bool debugMode,
                           unsigned int nthreads)
: Engine(adios, "BPFileReader", std::move(name), std::move(accessMode), mpiComm,
         method, debugMode, nthreads,
         " BPFileReader constructor (or call to ADIOS Open).\n"),
  m_Buffer(accessMode, m_RankMPI, m_DebugMode)
{
  Init();
}

Variable<void> *BPFileReader::InquireVariable(const std::string /*name*/,
                                              const bool /*readIn*/)
{
  // not yet implemented
  return nullptr;
}

Variable<char> *BPFileReader::InquireVariableChar(const std::string name,
                                                  const bool readIn)
{
  return InquireVariableCommon<char>(name, readIn);
}

Variable<unsigned char> *
BPFileReader::InquireVariableUChar(const std::string name, const bool readIn)
{
  return InquireVariableCommon<unsigned char>(name, readIn);
}

Variable<short> *BPFileReader::InquireVariableShort(const std::string name,
                                                    const bool readIn)
{
  return InquireVariableCommon<short>(name, readIn);
}

Variable<unsigned short> *
BPFileReader::InquireVariableUShort(const std::string name, const bool readIn)
{
  return InquireVariableCommon<unsigned short>(name, readIn);
}

Variable<int> *BPFileReader::InquireVariableInt(const std::string name,
                                                const bool readIn)
{
  return InquireVariableCommon<int>(name, readIn);
}

Variable<unsigned int> *
BPFileReader::InquireVariableUInt(const std::string name, const bool readIn)
{
  return InquireVariableCommon<unsigned int>(name, readIn);
}

Variable<long int> *BPFileReader::InquireVariableLInt(const std::string name,
                                                      const bool readIn)
{
  return InquireVariableCommon<long int>(name, readIn);
}

Variable<unsigned long int> *
BPFileReader::InquireVariableULInt(const std::string name, const bool readIn)
{
  return InquireVariableCommon<unsigned long int>(name, readIn);
}

Variable<long long int> *
BPFileReader::InquireVariableLLInt(const std::string name, const bool readIn)
{
  return InquireVariableCommon<long long int>(name, readIn);
}

Variable<unsigned long long int> *
BPFileReader::InquireVariableULLInt(const std::string name, const bool readIn)
{
  return InquireVariableCommon<unsigned long long int>(name, readIn);
}

Variable<float> *BPFileReader::InquireVariableFloat(const std::string name,
                                                    const bool readIn)
{
  return InquireVariableCommon<float>(name, readIn);
}

Variable<double> *BPFileReader::InquireVariableDouble(const std::string name,
                                                      const bool readIn)
{
  return InquireVariableCommon<double>(name, readIn);
}

Variable<long double> *
BPFileReader::InquireVariableLDouble(const std::string name, const bool readIn)
{
  return InquireVariableCommon<long double>(name, readIn);
}

Variable<std::complex<float>> *
BPFileReader::InquireVariableCFloat(const std::string name, const bool readIn)
{
  return InquireVariableCommon<std::complex<float>>(name, readIn);
}

Variable<std::complex<double>> *
BPFileReader::InquireVariableCDouble(const std::string name, const bool readIn)
{
  return InquireVariableCommon<std::complex<double>>(name, readIn);
}

Variable<std::complex<long double>> *
BPFileReader::InquireVariableCLDouble(const std::string name, const bool readIn)
{
  return InquireVariableCommon<std::complex<long double>>(name, readIn);
}

VariableCompound *
BPFileReader::InquireVariableCompound(const std::string /*name*/,
                                      const bool /*readIn*/)
{
  return nullptr;
}

void BPFileReader::Close(const int /*transportIndex*/) {}

// PRIVATE
void BPFileReader::Init()
{
  if (m_DebugMode == true)
  {
    if (m_AccessMode != "r" && m_AccessMode != "read")
    {
      throw std::invalid_argument(
          "ERROR: BPFileReader doesn't support access mode " + m_AccessMode +
          ", in call to ADIOS Open or BPFileReader constructor\n");
    }
  }

  InitCapsules();
  InitTransports();
}

void BPFileReader::InitCapsules()
{
  // here init memory capsules
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
        auto file =
            std::make_shared<transport::FileDescriptor>(m_MPIComm, m_DebugMode);
        // m_BP1Reader.OpenRankFiles( m_Name, m_AccessMode, *file );
        m_Transports.push_back(std::move(file));
      }
      else if (itLibrary->second == "FILE*" || itLibrary->second == "stdio.h")
      {
        auto file =
            std::make_shared<transport::FilePointer>(m_MPIComm, m_DebugMode);
        // m_BP1Reader.OpenRankFiles( m_Name, m_AccessMode, *file );
        m_Transports.push_back(std::move(file));
      }
      else if (itLibrary->second == "fstream" ||
               itLibrary->second == "std::fstream")
      {
        auto file =
            std::make_shared<transport::FStream>(m_MPIComm, m_DebugMode);
        // m_BP1Reader.OpenRankFiles( m_Name, m_AccessMode, *file );
        m_Transports.push_back(std::move(file));
      }
      else if (itLibrary->second == "MPI-IO")
      {
      }
      else
      {
        if (m_DebugMode == true)
        {
          throw std::invalid_argument(
              "ERROR: file transport library " + itLibrary->second +
              " not supported, in " + m_Name + m_EndMessage);
        }
      }
    }
    else
    {
      if (m_DebugMode == true)
      {
        throw std::invalid_argument("ERROR: transport " + itTransport->second +
                                    " (you mean File?) not supported, in " +
                                    m_Name + m_EndMessage);
      }
    }
  }
}

} // end namespace adios
