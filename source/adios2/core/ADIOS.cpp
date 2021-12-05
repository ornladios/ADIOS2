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
#include <fstream>
#include <ios> //std::ios_base::failure
#include <mutex>

#include "adios2/core/IO.h"
#include "adios2/helper/adiosCommDummy.h"
#include "adios2/helper/adiosFunctions.h" //InquireKey, BroadcastFile
#include "adios2/operator/compress/CompressorFactory.h"
#include <adios2sys/SystemTools.hxx>

#include <adios2-perfstubs-interface.h>

// callbacks
#include "adios2/operator/callback/Signature1.h"
#include "adios2/operator/callback/Signature2.h"

namespace adios2
{
namespace core
{

std::mutex PerfStubsMutex;

ADIOS::ADIOS(const std::string configFile, helper::Comm comm,
             const std::string hostLanguage)
: m_HostLanguage(hostLanguage), m_Comm(std::move(comm)),
  m_ConfigFile(configFile)
{
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
            throw std::logic_error("Config file " + configFile +
                                   " passed to ADIOS does not exist.");
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

ADIOS::~ADIOS() = default;

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
            throw std::invalid_argument(
                "ERROR: IO with name " + name +
                " previously declared with DeclareIO, name must be "
                "unique,"
                " in call to DeclareIO\n");
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
        throw std::invalid_argument("ERROR: IO with name " + name +
                                    " was not declared, did you previously "
                                    "call DeclareIO?, in call to AtIO\n");
    }
    else
    {
        if (!itIO->second.IsDeclared())
        {
            throw std::invalid_argument("ERROR: IO with name " + name +
                                        " was not declared, did you previously "
                                        "call DeclareIO ?, in call to AtIO\n");
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

Operator &ADIOS::DefineOperator(const std::string &name, const std::string type,
                                const Params &parameters)
{
    CheckOperator(name);
    auto itPair =
        m_Operators.emplace(name, compress::MakeOperator(type, parameters));
    return *itPair.first->second.get();
}

Operator *ADIOS::InquireOperator(const std::string &name) noexcept
{
    std::shared_ptr<Operator> *op = helper::InquireKey(name, m_Operators);
    if (op == nullptr)
    {
        return nullptr;
    }
    return op->get();
}

#define declare_type(T)                                                        \
    Operator &ADIOS::DefineCallBack(                                           \
        const std::string name,                                                \
        const std::function<void(const T *, const std::string &,               \
                                 const std::string &, const std::string &,     \
                                 const size_t, const Dims &, const Dims &,     \
                                 const Dims &)> &function,                     \
        const Params &parameters)                                              \
    {                                                                          \
        CheckOperator(name);                                                   \
        std::shared_ptr<Operator> callbackOperator =                           \
            std::make_shared<callback::Signature1>(function, parameters);      \
                                                                               \
        auto itPair = m_Operators.emplace(name, std::move(callbackOperator));  \
        return *itPair.first->second;                                          \
    }

ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

Operator &ADIOS::DefineCallBack(
    const std::string name,
    const std::function<void(void *, const std::string &, const std::string &,
                             const std::string &, const size_t, const Dims &,
                             const Dims &, const Dims &)> &function,
    const Params &parameters)
{
    CheckOperator(name);
    std::shared_ptr<Operator> callbackOperator =
        std::make_shared<callback::Signature2>(function, parameters);

    auto itPair = m_Operators.emplace(name, std::move(callbackOperator));
    return *itPair.first->second;
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
        throw std::invalid_argument(
            "ERROR: Operator with name " + name +
            ", is already defined in either config file "
            "or with call to DefineOperator, name must "
            "be unique, in call to DefineOperator\n");
    }
}

void ADIOS::XMLInit(const std::string &configFileXML)
{
    helper::ParseConfigXML(*this, configFileXML, m_IOs, m_Operators);
}

void ADIOS::YAMLInit(const std::string &configFileYAML)
{
    helper::ParseConfigYAML(*this, configFileYAML, m_IOs, m_Operators);
}

} // end namespace core
} // end namespace adios2
