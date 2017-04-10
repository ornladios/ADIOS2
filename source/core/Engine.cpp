/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Engine.cpp
 *
 *  Created on: Dec 19, 2016
 *      Author: wfg
 */

#include <ios> //std::ios_base::failure

#include "core/Engine.h"
#include "core/Support.h"
#include "functions/adiosFunctions.h"

namespace adios
{

Engine::Engine(ADIOS &adios, const std::string engineType,
               const std::string &name, const std::string accessMode,
               MPI_Comm mpiComm, const Method &method,
               const std::string endMessage)
: m_MPIComm(mpiComm), m_EngineType(engineType), m_Name(name),
  m_AccessMode(accessMode), m_Method(method), m_ADIOS(adios),
  m_DebugMode(m_Method.m_DebugMode), m_EndMessage(endMessage)
{
    if (m_DebugMode == true)
    {
        if (m_MPIComm == MPI_COMM_NULL)
        {
            throw std::ios_base::failure(
                "ERROR: engine communicator is MPI_COMM_NULL,"
                " in call to ADIOS Open or Constructor\n");
        }
    }

    MPI_Comm_rank(m_MPIComm, &m_RankMPI);
    MPI_Comm_size(m_MPIComm, &m_SizeMPI);
}

void Engine::SetCallBack(std::function<void(const void *, std::string,
                                            std::string, std::string, Dims)>
                             callback)
{
}

// should these functions throw an exception?
void Engine::Write(Variable<char> & /*variable*/, const char * /*values*/) {}
void Engine::Write(Variable<unsigned char> & /*variable*/,
                   const unsigned char * /*values*/)
{
}
void Engine::Write(Variable<short> & /*variable*/, const short * /*values*/) {}
void Engine::Write(Variable<unsigned short> & /*variable*/,
                   const unsigned short * /*values*/)
{
}
void Engine::Write(Variable<int> & /*variable*/, const int * /*values*/) {}
void Engine::Write(Variable<unsigned int> & /*variable*/,
                   const unsigned int * /*values*/)
{
}
void Engine::Write(Variable<long int> & /*variable*/,
                   const long int * /*values*/)
{
}
void Engine::Write(Variable<unsigned long int> & /*variable*/,
                   const unsigned long int * /*values*/)
{
}
void Engine::Write(Variable<long long int> & /*variable*/,
                   const long long int * /*values*/)
{
}
void Engine::Write(Variable<unsigned long long int> & /*variable*/,
                   const unsigned long long int * /*values*/)
{
}
void Engine::Write(Variable<float> & /*variable*/, const float * /*values*/) {}
void Engine::Write(Variable<double> & /*variable*/, const double * /*values*/)
{
}
void Engine::Write(Variable<long double> & /*variable*/,
                   const long double * /*values*/)
{
}
void Engine::Write(Variable<std::complex<float>> & /*variable*/,
                   const std::complex<float> * /*values*/)
{
}
void Engine::Write(Variable<std::complex<double>> & /*variable*/,
                   const std::complex<double> * /*values*/)
{
}
void Engine::Write(Variable<std::complex<long double>> & /*variable*/,
                   const std::complex<long double> * /*values*/)
{
}
void Engine::Write(VariableCompound & /*variable*/, const void * /*values*/) {}

void Engine::Write(const std::string & /*variableName*/,
                   const char * /*values*/)
{
}
void Engine::Write(const std::string & /*variableName*/,
                   const unsigned char * /*values*/)
{
}
void Engine::Write(const std::string & /*variableName*/,
                   const short * /*values*/)
{
}
void Engine::Write(const std::string & /*variableName*/,
                   const unsigned short * /*values*/)
{
}
void Engine::Write(const std::string & /*variableName*/, const int * /*values*/)
{
}
void Engine::Write(const std::string & /*variableName*/,
                   const unsigned int * /*values*/)
{
}
void Engine::Write(const std::string & /*variableName*/,
                   const long int * /*values*/)
{
}
void Engine::Write(const std::string & /*variableName*/,
                   const unsigned long int * /*values*/)
{
}
void Engine::Write(const std::string & /*variableName*/,
                   const long long int * /*values*/)
{
}
void Engine::Write(const std::string & /*variableName*/,
                   const unsigned long long int * /*values*/)
{
}
void Engine::Write(const std::string & /*variableName*/,
                   const float * /*values*/)
{
}
void Engine::Write(const std::string & /*variableName*/,
                   const double * /*values*/)
{
}
void Engine::Write(const std::string & /*variableName*/,
                   const long double * /*values*/)
{
}
void Engine::Write(const std::string & /*variableName*/,
                   const std::complex<float> * /*values*/)
{
}
void Engine::Write(const std::string & /*variableName*/,
                   const std::complex<double> * /*values*/)
{
}
void Engine::Write(const std::string & /*variableName*/,
                   const std::complex<long double> * /*values*/)
{
}
void Engine::Write(const std::string & /*variableName*/,
                   const void * /*values*/)
{
}

void Engine::Advance(float /*timeout_sec*/) {}
void Engine::Advance(AdvanceMode /*mode*/, float /*timeout_sec*/) {}
void Engine::AdvanceAsync(
    AdvanceMode /*mode*/,
    std::function<void(std::shared_ptr<adios::Engine>)> /*callback*/)
{
}

void Engine::Close(const int /*transportIndex*/) {}

// READ
Variable<void> *Engine::InquireVariable(const std::string & /*name*/,
                                        const bool /*readIn*/)
{
    return nullptr;
}
Variable<char> *Engine::InquireVariableChar(const std::string & /*name*/,
                                            const bool /*readIn*/)
{
    return nullptr;
}
Variable<unsigned char> *
Engine::InquireVariableUChar(const std::string & /*name*/,
                             const bool /*readIn*/)
{
    return nullptr;
}
Variable<short> *Engine::InquireVariableShort(const std::string & /*name*/,
                                              const bool /*readIn*/)
{
    return nullptr;
}
Variable<unsigned short> *
Engine::InquireVariableUShort(const std::string & /*name*/,
                              const bool /*readIn*/)
{
    return nullptr;
}
Variable<int> *Engine::InquireVariableInt(const std::string & /*name*/,
                                          const bool /*readIn*/)
{
    return nullptr;
}
Variable<unsigned int> *
Engine::InquireVariableUInt(const std::string & /*name*/, const bool /*readIn*/)
{
    return nullptr;
}
Variable<long int> *Engine::InquireVariableLInt(const std::string & /*name*/,
                                                const bool /*readIn*/)
{
    return nullptr;
}
Variable<unsigned long int> *
Engine::InquireVariableULInt(const std::string & /*name*/,
                             const bool /*readIn*/)
{
    return nullptr;
}
Variable<long long int> *
Engine::InquireVariableLLInt(const std::string & /*name*/,
                             const bool /*readIn*/)
{
    return nullptr;
}
Variable<unsigned long long int> *
Engine::InquireVariableULLInt(const std::string & /*name*/,
                              const bool /*readIn*/)
{
    return nullptr;
}
Variable<float> *Engine::InquireVariableFloat(const std::string & /*name*/,
                                              const bool /*readIn*/)
{
    return nullptr;
}
Variable<double> *Engine::InquireVariableDouble(const std::string & /*name*/,
                                                const bool /*readIn*/)
{
    return nullptr;
}
Variable<long double> *
Engine::InquireVariableLDouble(const std::string & /*name*/,
                               const bool /*readIn*/)
{
    return nullptr;
}
Variable<std::complex<float>> *
Engine::InquireVariableCFloat(const std::string & /*name*/,
                              const bool /*readIn*/)
{
    return nullptr;
}
Variable<std::complex<double>> *
Engine::InquireVariableCDouble(const std::string & /*name*/,
                               const bool /*readIn*/)
{
    return nullptr;
}
Variable<std::complex<long double>> *
Engine::InquireVariableCLDouble(const std::string & /*name*/,
                                const bool /*readIn*/)
{
    return nullptr;
}
VariableCompound *Engine::InquireVariableCompound(const std::string & /*name*/,
                                                  const bool /*readIn*/)
{
    return nullptr;
}

void Engine::Read(Variable<double> & /*variable*/, const double * /*values*/) {}
void Engine::ScheduleRead(Variable<double> & /*variable*/,
                          const double * /*values*/)
{
}
void Engine::Release() {}

// PROTECTED
void Engine::Init() {}

void Engine::InitParameters() {}

void Engine::InitTransports() {}

void Engine::CheckParameter(
    const std::map<std::string, std::string>::const_iterator itParameter,
    const std::map<std::string, std::string> &parameters,
    const std::string parameterName, const std::string hint) const
{
    if (itParameter == parameters.end())
    {
        {
            throw std::invalid_argument("ERROR: parameter name " +
                                        parameterName + " not found " + hint);
        }
    }
}

bool Engine::TransportNamesUniqueness() const
{
    auto lf_CheckTransportsType =
        [&](const std::set<std::string> &specificType) -> bool

    {
        std::set<std::string> transportNames;

        for (const auto &parameters : m_Method.m_TransportParameters)
        {
            auto itTransport = parameters.find("transport");
            if (m_DebugMode == true)
            {
                if (itTransport == parameters.end())
                {
                    throw std::invalid_argument("ERROR: transport not defined "
                                                "in Method input to Engine " +
                                                m_Name);
                }
            }

            const std::string type(itTransport->second);
            if (specificType.count(type) == 1) // file transports type
            {
                std::string name(m_Name);
                auto itName = parameters.find("name");
                if (itName != parameters.end())
                {
                    name = itName->second;
                }

                if (transportNames.count(name) == 0)
                {
                    transportNames.insert(name);
                }
                else
                {
                    return false;
                }
            }
        }
        return true;
    };

    return lf_CheckTransportsType(Support::FileTransports);
}

void Engine::CheckTransportIndex(const int transportIndex)
{
    if (m_DebugMode == true)
    {
        if (transportIndex >= static_cast<int>(m_Transports.size()) ||
            transportIndex < -1)
        {
            throw std::invalid_argument(
                "ERROR: transport index " + std::to_string(transportIndex) +
                " is out of range, in call to " + m_Name + "Close \n");
        }
    }
}

} // end namespace adios
