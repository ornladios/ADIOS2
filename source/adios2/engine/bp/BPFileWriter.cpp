/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BPFileWriter.cpp
 *
 *  Created on: Dec 19, 2016
 *      Author: wfg
 */

#include "BPFileWriter.h"

#include <utility>

#include "adios2/ADIOS.h"
#include "adios2/transport/file/FStream.h"
#include "adios2/transport/file/FileDescriptor.h"
#include "adios2/transport/file/FilePointer.h"

namespace adios
{

BPFileWriter::BPFileWriter(ADIOS &adios, const std::string &name,
                           const std::string accessMode, MPI_Comm mpiComm,
                           const Method &method)
: Engine(adios, "BPFileWriter", name, accessMode, mpiComm, method,
         " BPFileWriter constructor (or call to ADIOS Open).\n"),
  m_BP1Writer(mpiComm, m_DebugMode)
{
    Init();
}

BPFileWriter::~BPFileWriter() = default;

void BPFileWriter::Init()
{
    InitParameters();
    InitTransports();
    InitProcessGroup();
}

void BPFileWriter::Write(Variable<char> &variable, const char *values)
{
    WriteVariableCommon(variable, values);
}

void BPFileWriter::Write(Variable<unsigned char> &variable,
                         const unsigned char *values)
{
    WriteVariableCommon(variable, values);
}

void BPFileWriter::Write(Variable<short> &variable, const short *values)
{
    WriteVariableCommon(variable, values);
}

void BPFileWriter::Write(Variable<unsigned short> &variable,
                         const unsigned short *values)
{
    WriteVariableCommon(variable, values);
}

void BPFileWriter::Write(Variable<int> &variable, const int *values)
{
    WriteVariableCommon(variable, values);
}

void BPFileWriter::Write(Variable<unsigned int> &variable,
                         const unsigned int *values)
{
    WriteVariableCommon(variable, values);
}

void BPFileWriter::Write(Variable<long int> &variable, const long int *values)
{
    WriteVariableCommon(variable, values);
}

void BPFileWriter::Write(Variable<unsigned long int> &variable,
                         const unsigned long int *values)
{
    WriteVariableCommon(variable, values);
}

void BPFileWriter::Write(Variable<long long int> &variable,
                         const long long int *values)
{
    WriteVariableCommon(variable, values);
}

void BPFileWriter::Write(Variable<unsigned long long int> &variable,
                         const unsigned long long int *values)
{
    WriteVariableCommon(variable, values);
}

void BPFileWriter::Write(Variable<float> &variable, const float *values)
{
    WriteVariableCommon(variable, values);
}

void BPFileWriter::Write(Variable<double> &variable, const double *values)
{
    WriteVariableCommon(variable, values);
}

void BPFileWriter::Write(Variable<long double> &variable,
                         const long double *values)
{
    WriteVariableCommon(variable, values);
}

void BPFileWriter::Write(Variable<std::complex<float>> &variable,
                         const std::complex<float> *values)
{
    WriteVariableCommon(variable, values);
}

void BPFileWriter::Write(Variable<std::complex<double>> &variable,
                         const std::complex<double> *values)
{
    WriteVariableCommon(variable, values);
}

void BPFileWriter::Write(Variable<std::complex<long double>> &variable,
                         const std::complex<long double> *values)
{
    WriteVariableCommon(variable, values);
}

void BPFileWriter::Write(VariableCompound & /*variable*/,
                         const void * /*values*/)
{
}

// String version
void BPFileWriter::Write(const std::string &variableName, const char *values)
{
    WriteVariableCommon(m_ADIOS.GetVariable<char>(variableName), values);
}

void BPFileWriter::Write(const std::string &variableName,
                         const unsigned char *values)
{
    WriteVariableCommon(m_ADIOS.GetVariable<unsigned char>(variableName),
                        values);
}

void BPFileWriter::Write(const std::string &variableName, const short *values)
{
    WriteVariableCommon(m_ADIOS.GetVariable<short>(variableName), values);
}

void BPFileWriter::Write(const std::string &variableName,
                         const unsigned short *values)
{
    WriteVariableCommon(m_ADIOS.GetVariable<unsigned short>(variableName),
                        values);
}

void BPFileWriter::Write(const std::string &variableName, const int *values)
{
    WriteVariableCommon(m_ADIOS.GetVariable<int>(variableName), values);
}

void BPFileWriter::Write(const std::string &variableName,
                         const unsigned int *values)
{
    WriteVariableCommon(m_ADIOS.GetVariable<unsigned int>(variableName),
                        values);
}

void BPFileWriter::Write(const std::string &variableName,
                         const long int *values)
{
    WriteVariableCommon(m_ADIOS.GetVariable<long int>(variableName), values);
}

void BPFileWriter::Write(const std::string &variableName,
                         const unsigned long int *values)
{
    WriteVariableCommon(m_ADIOS.GetVariable<unsigned long int>(variableName),
                        values);
}

void BPFileWriter::Write(const std::string &variableName,
                         const long long int *values)
{
    WriteVariableCommon(m_ADIOS.GetVariable<long long int>(variableName),
                        values);
}

void BPFileWriter::Write(const std::string &variableName,
                         const unsigned long long int *values)
{
    WriteVariableCommon(
        m_ADIOS.GetVariable<unsigned long long int>(variableName), values);
}

void BPFileWriter::Write(const std::string &variableName, const float *values)
{
    WriteVariableCommon(m_ADIOS.GetVariable<float>(variableName), values);
}

void BPFileWriter::Write(const std::string &variableName, const double *values)
{
    WriteVariableCommon(m_ADIOS.GetVariable<double>(variableName), values);
}

void BPFileWriter::Write(const std::string &variableName,
                         const long double *values)
{
    WriteVariableCommon(m_ADIOS.GetVariable<long double>(variableName), values);
}

void BPFileWriter::Write(const std::string &variableName,
                         const std::complex<float> *values)
{
    WriteVariableCommon(m_ADIOS.GetVariable<std::complex<float>>(variableName),
                        values);
}

void BPFileWriter::Write(const std::string &variableName,
                         const std::complex<double> *values)
{
    WriteVariableCommon(m_ADIOS.GetVariable<std::complex<double>>(variableName),
                        values);
}

void BPFileWriter::Write(const std::string &variableName,
                         const std::complex<long double> *values)
{
    WriteVariableCommon(
        m_ADIOS.GetVariable<std::complex<long double>>(variableName), values);
}

void BPFileWriter::Write(const std::string & /*variableName*/,
                         const void * /*values*/) // Compound type
{
}

void BPFileWriter::Advance(float /*timeout_sec*/) { m_BP1Writer.Advance(); }

void BPFileWriter::Close(const int transportIndex)
{
    CheckTransportIndex(transportIndex);
    if (transportIndex == -1)
    {
        for (auto &transport : m_Transports)
        {
            // false: not using aggregation for now
            m_BP1Writer.Close(*transport, m_IsFirstClose, false);
        }
    }
    else
    {
        // false: not using aggregation for now
        m_BP1Writer.Close(*m_Transports[transportIndex], m_IsFirstClose, false);
    }

    if (m_BP1Writer.m_MetadataSet.Log.IsActive == true)
    {
        bool allClose = true;
        for (auto &transport : m_Transports)
        {
            if (transport->m_IsOpen == true)
            {
                allClose = false;
                break;
            }
        }

        if (allClose == true) // aggregate and write profiling.log
        {
            m_BP1Writer.DumpProfilingLogFile(m_Name, m_RankMPI, m_Transports);
        }
    }
}

// PRIVATE FUNCTIONS
void BPFileWriter::InitParameters()
{
    auto itProfile = m_Method.m_Parameters.find("profile_units");
    if (itProfile != m_Method.m_Parameters.end())
    {
        auto &log = m_BP1Writer.m_MetadataSet.Log;

        if (itProfile->second == "mus" || itProfile->second == "microseconds")
        {
            log.Timers.emplace_back("buffering", Support::Resolutions::mus);
        }
        else if (itProfile->second == "ms" ||
                 itProfile->second == "milliseconds")
        {
            log.Timers.emplace_back("buffering", Support::Resolutions::ms);
        }
        else if (itProfile->second == "s" || itProfile->second == "seconds")
        {
            log.Timers.emplace_back("buffering", Support::Resolutions::s);
        }
        else if (itProfile->second == "min" || itProfile->second == "minutes")
        {
            log.Timers.emplace_back("buffering", Support::Resolutions::m);
        }
        else if (itProfile->second == "h" || itProfile->second == "hours")
        {
            log.Timers.emplace_back("buffering", Support::Resolutions::h);
        }
        else
        {
            if (m_DebugMode == true)
            {
                throw std::invalid_argument(
                    "ERROR: Method profile_buffering_units "
                    "argument must be mus, ms, s, min or h, in "
                    "call to Open or Engine constructor\n");
            }
        }

        log.IsActive = true;
    }

    auto itGrowthFactor = m_Method.m_Parameters.find("buffer_growth");
    if (itGrowthFactor != m_Method.m_Parameters.end())
    {
        const float growthFactor = std::stof(itGrowthFactor->second);
        if (m_DebugMode == true)
        {
            if (growthFactor == 1.f)
            {
                throw std::invalid_argument("ERROR: buffer_growth argument "
                                            "can't be less of equal than 1, "
                                            "in " +
                                            m_EndMessage + "\n");
            }
        }

        m_BP1Writer.m_GrowthFactor = growthFactor;
    }

    auto itMaxBufferSize = m_Method.m_Parameters.find("max_size_MB");
    if (itMaxBufferSize != m_Method.m_Parameters.end())
    {
        if (m_DebugMode == true)
        {
            if (m_GrowthFactor <= 1.f)
            {
                throw std::invalid_argument(
                    "ERROR: Method buffer_growth argument "
                    "can't be less of equal than 1, in " +
                    m_EndMessage + "\n");
            }
        }

        // convert from MB to bytes
        m_BP1Writer.m_MaxBufferSize =
            std::stoul(itMaxBufferSize->second) * 1048576;
    }

    auto itVerbosity = m_Method.m_Parameters.find("verbose");
    if (itVerbosity != m_Method.m_Parameters.end())
    {
        int verbosity = std::stoi(itVerbosity->second);
        if (m_DebugMode == true)
        {
            if (verbosity < 0 || verbosity > 5)
            {
                throw std::invalid_argument(
                    "ERROR: Method verbose argument must be an "
                    "integer in the range [0,5], in call to "
                    "Open or Engine constructor\n");
            }
        }
        m_BP1Writer.m_Verbosity = verbosity;
    }
}

void BPFileWriter::InitTransports()
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

    bool setBuffer = false;

    for (const auto &parameters : m_Method.m_TransportParameters)
    {
        auto itProfile = parameters.find("profile_units");
        bool doProfiling = false;
        Support::Resolutions resolution =
            Support::Resolutions::s; // default is seconds
        if (itProfile != parameters.end())
        {
            if (itProfile->second == "mus" ||
                itProfile->second == "microseconds")
            {
                resolution = Support::Resolutions::mus;
            }
            else if (itProfile->second == "ms" ||
                     itProfile->second == "milliseconds")
            {
                resolution = Support::Resolutions::ms;
            }
            else if (itProfile->second == "s" || itProfile->second == "seconds")
            {
                resolution = Support::Resolutions::s;
            }
            else if (itProfile->second == "min" ||
                     itProfile->second == "minutes")
            {
                resolution = Support::Resolutions::m;
            }
            else if (itProfile->second == "h" || itProfile->second == "hours")
            {
                resolution = Support::Resolutions::h;
            }
            else
            {
                if (m_DebugMode == true)
                {
                    throw std::invalid_argument(
                        "ERROR: Transport profile_units argument "
                        "must be mus, ms, s, min or h " +
                        m_EndMessage);
                }
            }
            doProfiling = true;
        }

        auto itTransport = parameters.find("transport");

        if (itTransport->second == "file" || itTransport->second == "File")
        {
            auto itLibrary = parameters.find("library");
            if (itLibrary == parameters.end() ||
                itLibrary->second == "POSIX") // use default POSIX
            {
                auto file = std::make_shared<transport::FileDescriptor>(
                    m_MPIComm, m_DebugMode);
                if (doProfiling == true)
                {
                    file->InitProfiler(m_AccessMode, resolution);
                }

                m_BP1Writer.OpenRankFiles(m_Name, m_AccessMode, *file);
                m_Transports.push_back(std::move(file));
                setBuffer = true;
            }
            else if (itLibrary->second == "FILE*" ||
                     itLibrary->second == "stdio")
            {
                auto file = std::make_shared<transport::FilePointer>(
                    m_MPIComm, m_DebugMode);
                if (doProfiling == true)
                {
                    file->InitProfiler(m_AccessMode, resolution);
                }

                m_BP1Writer.OpenRankFiles(m_Name, m_AccessMode, *file);
                m_Transports.push_back(std::move(file));
                setBuffer = true;
            }
            else if (itLibrary->second == "fstream" ||
                     itLibrary->second == "std::fstream")
            {
                auto file = std::make_shared<transport::FStream>(m_MPIComm,
                                                                 m_DebugMode);

                if (doProfiling == true)
                {
                    file->InitProfiler(m_AccessMode, resolution);
                }

                m_BP1Writer.OpenRankFiles(m_Name, m_AccessMode, *file);
                m_Transports.push_back(std::move(file));
                setBuffer = true;
            }
            else if (itLibrary->second == "MPI_File" ||
                     itLibrary->second == "MPI-IO")
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
                throw std::invalid_argument(
                    "ERROR: transport " + itTransport->second +
                    " (you mean File?) not supported, in " + m_Name +
                    m_EndMessage);
            }
        }
    }

    if (setBuffer == false)
    {
        if (m_DebugMode == true)
        {
            throw std::invalid_argument(
                "ERROR: file transport not declared in Method "
                "need call to Method.AddTransport, in " +
                m_Name + m_EndMessage);
        }
    }

    // initial size is 16KB, memory is initialized to zero
    m_BP1Writer.m_Heap.ResizeData(16777216);
}

void BPFileWriter::InitProcessGroup()
{
    if (m_BP1Writer.m_MetadataSet.Log.IsActive == true)
    {
        m_BP1Writer.m_MetadataSet.Log.Timers[0].SetInitialTime();
    }

    if (m_AccessMode == "a")
    {
        // Get last pg timestep and update timestep counter in
        // format::BP1MetadataSet
    }

    WriteProcessGroupIndex();

    if (m_BP1Writer.m_MetadataSet.Log.IsActive == true)
    {
        m_BP1Writer.m_MetadataSet.Log.Timers[0].SetTime();
    }
}

void BPFileWriter::WriteProcessGroupIndex()
{
    const bool isFortran = (m_HostLanguage == "Fortran") ? true : false;

    m_BP1Writer.WriteProcessGroupIndex(isFortran, std::to_string(m_RankMPI),
                                       static_cast<uint32_t>(m_RankMPI),
                                       m_Transports);
}

} // end namespace adios
