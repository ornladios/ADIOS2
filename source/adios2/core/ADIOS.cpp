/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS.cpp
 *
 *  Created on: Sep 29, 2016
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "ADIOS.h"

/// \cond EXCLUDE_FROM_DOXYGEN
#include <fstream>
#include <ios> //std::ios_base::failure
#include <iostream>
#include <sstream>
#include <utility>
/// \endcond

#include "adios2/ADIOSMPI.h"
#include "adios2/helper/adiosFunctions.h"

// transforms
#ifdef ADIOS2_HAVE_BZIP2
#include "adios2/transform/compress/CompressBZip2.h"
#endif

#ifdef ADIOS2_HAVE_ZFP
#include "adios2/transform/compress/CompressZfp.h"
#endif

namespace adios2
{

ADIOS::ADIOS(const std::string configFile, MPI_Comm mpiComm,
             const bool debugMode)
: m_MPIComm(mpiComm), m_ConfigFile(configFile), m_DebugMode(debugMode)
{
    if (m_DebugMode)
    {
        CheckMPI();
    }
    if (!configFile.empty())
    {
        if (configFile.substr(configFile.size() - 3) == "xml")
        {
            InitXML(configFile, m_MPIComm, m_DebugMode, m_Transforms, m_IOs);
        }
        // TODO expand for other formats
    }
}

ADIOS::ADIOS(const std::string configFile, const bool debugMode)
: ADIOS(configFile, MPI_COMM_SELF, debugMode)
{
}

ADIOS::ADIOS(MPI_Comm mpiComm, const bool debugMode)
: ADIOS("", mpiComm, debugMode)
{
}

ADIOS::ADIOS(const bool debugMode) : ADIOS("", MPI_COMM_SELF, debugMode) {}

IO &ADIOS::DeclareIO(const std::string ioName)
{
    auto itIO = m_IOs.find(ioName);

    if (itIO != m_IOs.end()) // exists
    {
        if (m_DebugMode)
        {
            if (!itIO->second.InConfigFile())
            {
                throw std::invalid_argument(
                    "ERROR: IO class object with name " + ioName +
                    " previously declared, name must be unique "
                    " , in call to DeclareIO\n");
            }
        }
        return itIO->second;
    }

    // doesn't exist, then create new pair
    auto ioPair =
        m_IOs.emplace(ioName, IO(ioName, m_MPIComm, false, m_DebugMode));
    return ioPair.first->second;
}

IO &ADIOS::GetIO(const std::string name)
{
    auto itIO = m_IOs.find(name);
    if (m_DebugMode && itIO == m_IOs.end())
    {
        throw std::invalid_argument(
            "ERROR: Unable to find previously defined IO object with name \"" +
            name + "\", in call to GetIO.");
    }
    return itIO->second;
}

Transform &ADIOS::GetTransform(const std::string transform)
{
    auto itTransform = m_Transforms.find(transform);

    if (itTransform != m_Transforms.end())
    {
        return *itTransform->second.get();
    }

    if (transform == "bzip2" || transform == "BZip2")
    {
#ifdef ADIOS2_HAVE_BZIP2
        auto itPair = m_Transforms.emplace(
            "bzip2",
            std::make_shared<adios2::transform::CompressBZip2>(m_DebugMode));
        return *itPair.first->second;
#else
        throw std::invalid_argument(
            "ERROR: this version of ADIOS2 didn't compile with "
            "bzip2 library, in call to GetTransport\n");
#endif
    }
    else if (transform == "zfp" || transform == "Zfp")
    {
#ifdef ADIOS2_HAVE_ZFP
        auto itPair = m_Transforms.emplace(
            "zfp",
            std::make_shared<adios2::transform::CompressZfp>(m_DebugMode));
        return *itPair.first->second;
#else
        throw std::invalid_argument(
            "ERROR: this version of ADIOS2 didn't compile with "
            "zfp library, in call to GetTransport\n");
#endif
    }
    else
    {
        if (m_DebugMode)
        {
            throw std::invalid_argument(
                "ERROR: transform " + transform +
                " not supported by ADIOS2, in call to GetTransport\n");
        }
    }

    return *itTransform->second.get();
}

// PRIVATE FUNCTIONS
void ADIOS::CheckMPI() const
{
    if (m_MPIComm == MPI_COMM_NULL)
    {
        throw std::ios_base::failure("ERROR: MPI communicator is MPI_COMM_NULL,"
                                     " in call to ADIOS constructor\n");
    }
}

} // end namespace adios2
