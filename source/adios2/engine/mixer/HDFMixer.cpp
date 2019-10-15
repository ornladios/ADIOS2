/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * HDFMixer.cpp
 *
 *  Created on: Dec 19, 2016
 *      Author: Junmin GU
 */

#include "HDFMixer.h"
#include "HDFMixer.tcc"

#include "adios2/core/IO.h"
#include "adios2/helper/adiosFunctions.h" //CheckIndexRange
#include "adios2/toolkit/transport/file/FileFStream.h"

namespace adios2
{
namespace core
{
namespace engine
{

HDFMixer::HDFMixer(IO &io, const std::string &name, const Mode openMode,
                   helper::Comm comm)
: Engine("HDFMixer", io, name, openMode, std::move(comm)),
  m_HDFVDSWriter(m_Comm, m_DebugMode),
  m_HDFSerialWriter(helper::Comm(), m_DebugMode),
  m_TransportsManager(m_Comm, m_DebugMode)
{
    m_EndMessage = " in call to IO Open HDFMixer " + m_Name + "\n";
    Init();
}

HDFMixer::~HDFMixer() = default;

void HDFMixer::Init()
{
    InitParameters();
    InitTransports();
    InitBuffer();
}

#define declare_type(T)                                                        \
    void HDFMixer::DoPutSync(Variable<T> &variable, const T *values)           \
    {                                                                          \
        DoPutSyncCommon(variable, values);                                     \
    }                                                                          \
    void HDFMixer::DoPutDeferred(Variable<T> &variable, const T *values)       \
    {                                                                          \
        DoPutSyncCommon(variable, values);                                     \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

StepStatus HDFMixer::BeginStep(StepMode mode, const float timeout_sec)
{
    return StepStatus::OK;
}

void HDFMixer::PerformPuts() {}

// void HDFMixer::Advance(const float /*timeout_sec*/)
// void HDFMixer::EndStep(const float /*timeout_sec*/)
void HDFMixer::EndStep()
{
    m_HDFSerialWriter.Advance();
    m_HDFVDSWriter.Advance();
}

// PRIVATE FUNCTIONS
void HDFMixer::InitParameters()
{
    // no need for hdf5
    // m_HDFSerialWriter.InitParameters(m_IO.m_Parameters);
}

void HDFMixer::InitTransports()
{
    if (m_IO.m_TransportsParameters.empty())
    {
        Params defaultTransportParameters;
        defaultTransportParameters["transport"] = "File";
        m_IO.m_TransportsParameters.push_back(defaultTransportParameters);
    }

#ifdef NEVER
/*
  // TODO need to add support for aggregators here later

  // Names are std::vector<std::string>
  auto transportsNames = m_TransportsManager.GetFilesBaseNames(
      m_Name, m_IO.m_TransportsParameters);
  auto bpBaseNames = m_HDFSerialWriter.GetBPBaseNames(transportsNames);
  auto bpNames = m_HDFSerialWriter.GetBPNames(transportsNames);

  m_TransportsManager.OpenFiles(bpBaseNames, bpNames, m_OpenMode,
                                m_IO.m_TransportsParameters,
                                m_HDFSerialWriter.m_Profiler.IsActive);
*/
#else

    m_HDFSerialWriter.Init(m_Name, m_Comm.Rank());
    m_HDFVDSWriter.Init(m_Name);
/*
auto transportsNames = m_TransportsManager.GetFilesBaseNames(
                                                             m_Name,
m_IO.m_TransportsParameters); auto baseNames =
m_HDFSerialWriter.GetBaseNames(transportsNames);

auto h5name =  m_HDFSerialWriter.GetLocalFileNames(baseNames, m_Name); // e.g.
base=/my/path/xy_1.h5 m_TransportsManager.OpenFiles(baseNames, h5name,
m_OpenMode, m_IO.m_TransportsParameters, m_HDFSerialWriter.m_Profiler.IsActive);
*/
#endif
}

void HDFMixer::InitBuffer()
{
    /*
      if (m_OpenMode == OpenMode::Append)
      {
          throw std::invalid_argument(
              "ADIOS2: OpenMode Append hasn't been implemented, yet");
          // TODO: Get last pg timestep and update timestep counter in
      }
      else
      {
        m_HDFSerialWriter.WriteProcessGroupIndex(
                                                 m_IO.m_HostLanguage,
      m_TransportsManager.GetTransportsTypes());
      }
    */
}

void HDFMixer::WriteProfilingJSONFile() {}

void HDFMixer::DoClose(const int transportIndex)
{
    m_HDFSerialWriter.Close();
    m_HDFVDSWriter.Close();
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
