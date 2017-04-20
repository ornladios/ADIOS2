/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataMan.cpp
 *
 *  Created on: Jan 10, 2017
 *      Author: wfg
 */

#include "DataManWriter.h"

#include <iostream> //needs to go away, this is just for demo purposes

#include "adios2/core/Support.h"
#include "adios2/core/adiosFunctions.h"    //CSVToVector
#include "adios2/transport/file/FStream.h" // uses C++ fstream
#include "adios2/transport/wan/MdtmMan.h"  //uses Mdtm library

namespace adios
{

DataManWriter::DataManWriter(ADIOS &adios, const std::string name,
                             const std::string accessMode, MPI_Comm mpiComm,
                             const Method &method)
: Engine(adios, "DataManWriter", name, accessMode, mpiComm, method,
         " DataManWriter constructor (or call to ADIOS Open).\n")
{
    Init();
}

void DataManWriter::SetCallBack(
    std::function<void(const void *, std::string, std::string, std::string,
                       Dims)>
        callback)
{
    m_CallBack = callback;
    m_Man.reg_callback(callback);
}

void DataManWriter::Write(Variable<char> &variable, const char *values)
{
    WriteVariableCommon(variable, values);
}

void DataManWriter::Write(Variable<unsigned char> &variable,
                          const unsigned char *values)
{
    WriteVariableCommon(variable, values);
}

void DataManWriter::Write(Variable<short> &variable, const short *values)
{
    WriteVariableCommon(variable, values);
}

void DataManWriter::Write(Variable<unsigned short> &variable,
                          const unsigned short *values)
{
    WriteVariableCommon(variable, values);
}

void DataManWriter::Write(Variable<int> &variable, const int *values)
{
    WriteVariableCommon(variable, values);
}

void DataManWriter::Write(Variable<unsigned int> &variable,
                          const unsigned int *values)
{
    WriteVariableCommon(variable, values);
}

void DataManWriter::Write(Variable<long int> &variable, const long int *values)
{
    WriteVariableCommon(variable, values);
}

void DataManWriter::Write(Variable<unsigned long int> &variable,
                          const unsigned long int *values)
{
    WriteVariableCommon(variable, values);
}

void DataManWriter::Write(Variable<long long int> &variable,
                          const long long int *values)
{
    WriteVariableCommon(variable, values);
}

void DataManWriter::Write(Variable<unsigned long long int> &variable,
                          const unsigned long long int *values)
{
    WriteVariableCommon(variable, values);
}

void DataManWriter::Write(Variable<float> &variable, const float *values)
{
    WriteVariableCommon(variable, values);
}

void DataManWriter::Write(Variable<double> &variable, const double *values)
{
    WriteVariableCommon(variable, values);
}

void DataManWriter::Write(Variable<long double> &variable,
                          const long double *values)
{
    WriteVariableCommon(variable, values);
}

void DataManWriter::Write(Variable<std::complex<float>> &variable,
                          const std::complex<float> *values)
{
    WriteVariableCommon(variable, values);
}

void DataManWriter::Write(Variable<std::complex<double>> &variable,
                          const std::complex<double> *values)
{
    WriteVariableCommon(variable, values);
}

void DataManWriter::Write(Variable<std::complex<long double>> &variable,
                          const std::complex<long double> *values)
{
    WriteVariableCommon(variable, values);
}

// String version
void DataManWriter::Write(const std::string &variableName, const char *values)
{
    WriteVariableCommon(m_ADIOS.GetVariable<char>(variableName), values);
}

void DataManWriter::Write(const std::string &variableName,
                          const unsigned char *values)
{
    WriteVariableCommon(m_ADIOS.GetVariable<unsigned char>(variableName),
                        values);
}

void DataManWriter::Write(const std::string &variableName, const short *values)
{
    WriteVariableCommon(m_ADIOS.GetVariable<short>(variableName), values);
}

void DataManWriter::Write(const std::string &variableName,
                          const unsigned short *values)
{
    WriteVariableCommon(m_ADIOS.GetVariable<unsigned short>(variableName),
                        values);
}

void DataManWriter::Write(const std::string &variableName, const int *values)
{
    WriteVariableCommon(m_ADIOS.GetVariable<int>(variableName), values);
}

void DataManWriter::Write(const std::string &variableName,
                          const unsigned int *values)
{
    WriteVariableCommon(m_ADIOS.GetVariable<unsigned int>(variableName),
                        values);
}

void DataManWriter::Write(const std::string &variableName,
                          const long int *values)
{
    WriteVariableCommon(m_ADIOS.GetVariable<long int>(variableName), values);
}

void DataManWriter::Write(const std::string &variableName,
                          const unsigned long int *values)
{
    WriteVariableCommon(m_ADIOS.GetVariable<unsigned long int>(variableName),
                        values);
}

void DataManWriter::Write(const std::string &variableName,
                          const long long int *values)
{
    WriteVariableCommon(m_ADIOS.GetVariable<long long int>(variableName),
                        values);
}

void DataManWriter::Write(const std::string &variableName,
                          const unsigned long long int *values)
{
    WriteVariableCommon(
        m_ADIOS.GetVariable<unsigned long long int>(variableName), values);
}

void DataManWriter::Write(const std::string &variableName, const float *values)
{
    WriteVariableCommon(m_ADIOS.GetVariable<float>(variableName), values);
}

void DataManWriter::Write(const std::string &variableName, const double *values)
{
    WriteVariableCommon(m_ADIOS.GetVariable<double>(variableName), values);
}

void DataManWriter::Write(const std::string &variableName,
                          const long double *values)
{
    WriteVariableCommon(m_ADIOS.GetVariable<long double>(variableName), values);
}

void DataManWriter::Write(const std::string &variableName,
                          const std::complex<float> *values)
{
    WriteVariableCommon(m_ADIOS.GetVariable<std::complex<float>>(variableName),
                        values);
}

void DataManWriter::Write(const std::string &variableName,
                          const std::complex<double> *values)
{
    WriteVariableCommon(m_ADIOS.GetVariable<std::complex<double>>(variableName),
                        values);
}

void DataManWriter::Write(const std::string &variableName,
                          const std::complex<long double> *values)
{
    WriteVariableCommon(
        m_ADIOS.GetVariable<std::complex<long double>>(variableName), values);
}

void DataManWriter::Close(const int transportIndex)
{
    m_Man.flush();
    // here close IPs and deallocate or free/close resources (if using STL no
    // need
    // for memory deallocation)
}

// PRIVATE functions below
void DataManWriter::Init()
{
    if (m_DebugMode == true)
    {
        if (m_AccessMode != "w" && m_AccessMode != "write" &&
            m_AccessMode != "a" && m_AccessMode != "append")
            throw std::invalid_argument(
                "ERROR: DataManWriter doesn't support access mode " +
                m_AccessMode +
                ", in call to ADIOS Open or DataManWriter constructor\n");
    }

    auto itRealTime = m_Method.m_Parameters.find("real_time");
    if (itRealTime != m_Method.m_Parameters.end())
    {
        if (itRealTime->second == "yes" || itRealTime->second == "true")
            m_DoRealTime = true;
    }

    itRealTime = m_Method.m_Parameters.find("monitoring");
    if (itRealTime != m_Method.m_Parameters.end())
    {
        if (itRealTime->second == "yes" || itRealTime->second == "true")
            m_DoMonitor = true;
    }

    if (m_DoRealTime)
    {
        /**
         * Lambda function that assigns a parameter in m_Method to a
         * localVariable
         * of type std::string
         */
        auto lf_AssignString = [this](const std::string parameter,
                                      std::string &localVariable) {
            auto it = m_Method.m_Parameters.find(parameter);
            if (it != m_Method.m_Parameters.end())
            {
                localVariable = it->second;
            }
        };

        /**
         * Lambda function that assigns a parameter in m_Method to a
         * localVariable
         * of type int
         */
        auto lf_AssignInt = [this](const std::string parameter,
                                   int &localVariable) {
            auto it = m_Method.m_Parameters.find(parameter);
            if (it != m_Method.m_Parameters.end())
            {
                localVariable = std::stoi(it->second);
            }
        };

        auto is_number = [](const std::string &s) {
            return !s.empty() && std::find_if(s.begin(), s.end(), [](char c) {
                                     return !std::isdigit(c);
                                 }) == s.end();
        };

        json jmsg;
        for (auto &i : m_Method.m_Parameters)
        {
            if (is_number(i.second))
            {
                jmsg[i.first] = std::stoi(i.second);
            }
            else
            {
                jmsg[i.first] = i.second;
            }
        }
        jmsg["stream_mode"] = "sender";
        m_Man.add_stream(jmsg);

        std::string method_type;
        int num_channels = 0;
        lf_AssignString("method_type", method_type);
        lf_AssignInt("num_channels", num_channels);
    }
    else
    {
        InitTransports();
    }
}

void DataManWriter::InitTransports() // maybe move this?
{
    TransportNamesUniqueness();

    for (const auto &parameters : m_Method.m_TransportParameters)
    {
        auto itTransport = parameters.find("transport");

        if (itTransport->second == "Mdtm" || itTransport->second == "MdtmMan")
        {
            const std::string localIP(
                GetMdtmParameter("localIP", parameters)); // mandatory
            const std::string remoteIP(
                GetMdtmParameter("remoteIP", parameters)); // mandatory
            const std::string prefix(GetMdtmParameter("prefix", parameters));
            const int numberOfPipes =
                std::stoi(GetMdtmParameter("pipes", parameters));
            const std::vector<int> tolerances =
                CSVToVectorInt(GetMdtmParameter("tolerances", parameters));
            const std::vector<int> priorities =
                CSVToVectorInt(GetMdtmParameter("priorities", parameters));

            //            m_Transports.push_back(std::make_shared<transport::MdtmMan>(
            //                localIP, remoteIP, m_AccessMode, prefix,
            //                numberOfPipes,
            //                tolerances, priorities, m_MPIComm, m_DebugMode));
        }
        else if (itTransport->second == "Zmq")
        {
        }
        else
        {
            if (m_DebugMode == true)
                throw std::invalid_argument(
                    "ERROR: transport + " + itTransport->second +
                    " not supported, in " + m_Name + m_EndMessage);
        }
    }
}

std::string DataManWriter::GetMdtmParameter(
    const std::string parameter,
    const std::map<std::string, std::string> &mdtmParameters)
{
    auto itParam = mdtmParameters.find(parameter);
    if (itParam != mdtmParameters.end()) // found
    {
        return itParam->second; // return value
    }
    // if not found
    // mandatory ones
    if (parameter == "localIP" || parameter == "remoteIP")
    {
        if (m_DebugMode == true)
            throw std::invalid_argument(
                "ERROR: " + parameter +
                " parameter not found in Method, in call to "
                "DataManWriter constructor\n");
    }
    else if (parameter == "prefix")
    {
        return "";
    }
    else if (parameter == "pipes")
    {
        return "0"; // or 1?
    }
    else if (parameter == "tolerances") // so far empty string
    {
    }
    else if (parameter == "priority")
    {
    }

    return ""; // return empty string
}

} // end namespace adios
