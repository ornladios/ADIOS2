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

#include <algorithm> // std::transform
#include <atomic>
#include <fstream>
#include <ios> //std::ios_base::failure
#include <mutex>

#include "adios2/core/IO.h"
#include "adios2/helper/adiosCommDummy.h"
#include "adios2/helper/adiosFunctions.h" //InquireKey, BroadcastFile
#include "adios2/operator/OperatorFactory.h"
#include <adios2sys/SystemTools.hxx>

#include <adios2-perfstubs-interface.h>

// callbacks
#include "adios2/operator/callback/Signature1.h"
#include "adios2/operator/callback/Signature2.h"

#ifdef ADIOS2_HAVE_AWSSDK
#include <aws/core/Aws.h>
#include <aws/core/utils/logging/LogLevel.h>
Aws::SDKOptions awdSDKOptions;
#endif

namespace adios2
{
namespace core
{

class ADIOS::GlobalServices
{
public:
    GlobalServices() {}

    ~GlobalServices() {}

    void CheckStatus()
    {
        if (wasGlobalShutdown)
        {
            helper::Throw<std::logic_error>(
                "Core", "ADIOS::GlobalServices", "CheckStatus",
                "Global Services was already shutdown. Make sure there is one "
                "true global ADIOS object that is created first and destructed "
                "last to ensure Global services are initialized only once");
        }
    }

    void Finalize()
    {
#ifdef ADIOS2_HAVE_AWSSDK
        if (isAWSInitialized)
        {
            Aws::ShutdownAPI(options);
            isAWSInitialized = false;
        }
#endif
        wasGlobalShutdown = true;
    }

#ifdef ADIOS2_HAVE_AWSSDK
    void Init_AWS_API()
    {
        if (!isAWSInitialized)
        {
            options.loggingOptions.logLevel =
                Aws::Utils::Logging::LogLevel::Debug;
            Aws::InitAPI(options);
            isAWSInitialized = true;
        }
    }
    Aws::SDKOptions options;
    bool isAWSInitialized = false;
#endif

    bool wasGlobalShutdown = false;
};

ADIOS::GlobalServices ADIOS::m_GlobalServices;

std::mutex PerfStubsMutex;
static std::atomic_uint adios_refcount(0);

ADIOS::ADIOS(const std::string configFile, helper::Comm comm,
             const std::string hostLanguage)
: m_HostLanguage(hostLanguage), m_Comm(std::move(comm)),
  m_ConfigFile(configFile)
{
    ++adios_refcount;
#ifdef PERFSTUBS_USE_TIMERS
    {
        std::lock_guard<std::mutex> lck(PerfStubsMutex);
        static bool perfstubsInit(false);
        if (!perfstubsInit)
        {
            PERFSTUBS_INITIALIZE();
            perfstubsInit = true;
            atexit(ps_finalize_);
        }
    }
#endif
    if (!configFile.empty())
    {
        if (!adios2sys::SystemTools::FileExists(configFile))
        {
            helper::Throw<std::logic_error>("Core", "ADIOS", "ADIOS",
                                            "config file " + configFile +
                                                " not found");
        }
        if (helper::EndsWith(configFile, ".xml"))
        {
            XMLInit(configFile);
        }
        else if (helper::EndsWith(configFile, ".yaml") ||
                 helper::EndsWith(configFile, ".yml"))
        {
            YAMLInit(configFile);
        }
    }
}

ADIOS::ADIOS(const std::string configFile, const std::string hostLanguage)
: ADIOS(configFile, helper::CommDummy(), hostLanguage)
{
}

ADIOS::ADIOS(helper::Comm comm, const std::string hostLanguage)
: ADIOS("", std::move(comm), hostLanguage)
{
}

ADIOS::ADIOS(const std::string hostLanguage)
: ADIOS("", helper::CommDummy(), hostLanguage)
{
}

ADIOS::~ADIOS()
{
    --adios_refcount;
    if (!adios_refcount)
    {
        m_GlobalServices.Finalize();
    }
}

IO &ADIOS::DeclareIO(const std::string name, const ArrayOrdering ArrayOrder)
{
    auto itIO = m_IOs.find(name);

    if (itIO != m_IOs.end())
    {
        IO &io = itIO->second;

        if (!io.IsDeclared()) // exists from config xml
        {
            io.SetDeclared();
            io.SetArrayOrder(ArrayOrder);
            return io;
        }
        else
        {
            helper::Throw<std::invalid_argument>(
                "Core", "ADIOS", "DeclareIO", "IO " + name + " declared twice");
        }
    }

    auto ioPair = m_IOs.emplace(
        std::piecewise_construct, std::forward_as_tuple(name),
        std::forward_as_tuple(*this, name, false, m_HostLanguage));
    IO &io = ioPair.first->second;
    io.SetDeclared();
    io.SetArrayOrder(ArrayOrder);
    return io;
}

IO &ADIOS::AtIO(const std::string name)
{
    auto itIO = m_IOs.find(name);

    if (itIO == m_IOs.end())
    {
        helper::Throw<std::invalid_argument>("Core", "ADIOS", "AtIO",
                                             "IO " + name +
                                                 " being used is not declared");
    }
    else
    {
        if (!itIO->second.IsDeclared())
        {
            helper::Throw<std::invalid_argument>(
                "Core", "ADIOS", "AtIO",
                "IO " + name + " being used is not declared");
        }
    }

    return itIO->second;
}

void ADIOS::FlushAll()
{
    for (auto &ioPair : m_IOs)
    {
        ioPair.second.FlushAll();
    }
}

void ADIOS::EnterComputationBlock() noexcept
{
    enteredComputationBlock = true;
    for (auto &ioPair : m_IOs)
    {
        ioPair.second.EnterComputationBlock();
    }
}

void ADIOS::ExitComputationBlock() noexcept
{
    if (enteredComputationBlock)
    {
        enteredComputationBlock = false;
        for (auto &ioPair : m_IOs)
        {
            ioPair.second.ExitComputationBlock();
        }
    }
}

std::pair<std::string, Params> &ADIOS::DefineOperator(const std::string &name,
                                                      const std::string type,
                                                      const Params &parameters)
{
    CheckOperator(name);
    MakeOperator(type, parameters);
    m_Operators[name] = {type, parameters};
    return m_Operators[name];
}

std::pair<std::string, Params> *
ADIOS::InquireOperator(const std::string &name) noexcept
{
    auto it = m_Operators.find(name);
    if (it == m_Operators.end())
    {
        return nullptr;
    }
    else
    {
        return &it->second;
    }
}

bool ADIOS::RemoveIO(const std::string name)
{
    if (m_IOs.erase(name) == 1)
    {
        return true;
    }

    return false;
}

void ADIOS::RemoveAllIOs() noexcept { m_IOs.clear(); }

// PRIVATE FUNCTIONS
void ADIOS::CheckOperator(const std::string name) const
{
    if (m_Operators.count(name) == 1)
    {
        helper::Throw<std::invalid_argument>("Core", "ADIOS", "CheckOperator",
                                             "Operator " + name +
                                                 " defined twice");
    }
}

void ADIOS::XMLInit(const std::string &configFileXML)
{
    helper::ParseConfigXML(*this, configFileXML, m_IOs, m_Operators);
}

void ADIOS::YAMLInit(const std::string &configFileYAML)
{
    helper::ParseConfigYAML(*this, configFileYAML, m_IOs);
}

void ADIOS::Global_init_AWS_API()
{
    m_GlobalServices.CheckStatus();
#ifdef ADIOS2_HAVE_AWSSDK
    m_GlobalServices.Init_AWS_API();
#endif
}

} // end namespace core
} // end namespace adios2
