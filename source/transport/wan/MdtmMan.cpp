/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * MdtmMan.cpp
 *
 *  Created on: Jan 22, 2017
 *      Author: wfg
 */

#include "transport/wan/MdtmMan.h"

namespace adios
{
namespace transport
{

MdtmMan::MdtmMan(const std::string localIP, const std::string remoteIP,
                 const std::string mode, const std::string prefix,
                 const int numberOfPipes, const std::vector<int> tolerances,
                 const std::vector<int> priorities, MPI_Comm mpiComm,
                 const bool debugMode)
: Transport("File", mpiComm, debugMode), m_LocalIP{localIP},
  m_RemoteIP{remoteIP}, m_Mode{mode}, m_Prefix{prefix},
  m_NumberOfPipes{numberOfPipes}, m_Tolerances{tolerances},
  m_Priorities{priorities}
{
}

void MdtmMan::Open(const std::string name, const std::string accessMode) {}

void MdtmMan::SetBuffer(char *buffer, std::size_t size) {}

void MdtmMan::Write(const char *buffer, std::size_t size) {}

void MdtmMan::Flush() {}

void MdtmMan::Close() { m_NumberOfPipes = -1; }

// PRIVATE Functions
int MdtmMan::Put(const void *data, const std::string doid,
                 const std::string variable, const std::string dType,
                 const std::vector<std::uint64_t> &putShape,
                 const std::vector<uint64_t> &varShape,
                 const std::vector<uint64_t> &offset,
                 const std::uint64_t timestep, const int tolerance,
                 const int priority)
{

    return 0;
}

int MdtmMan::Get(void *data, const std::string doid, const std::string variable,
                 const std::string dType,
                 const std::vector<std::uint64_t> &putShape,
                 const std::vector<uint64_t> &varShape,
                 const std::vector<uint64_t> &offset,
                 const std::uint64_t timestep, const int tolerance,
                 const int priority)
{

    return 0;
}

int MdtmMan::Get(void *data, const std::string doid, const std::string variable,
                 const std::string dType, std::vector<std::uint64_t> &varShape,
                 const std::uint64_t timestep)
{

    return 0;
}

void MdtmMan::OnReceive(nlohmann::json &jData) {}

} // end namespace transport
} // end namespace
