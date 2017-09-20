/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataManReader.cpp
 *
 *  Created on: Feb 21, 2017
 *      Author: wfg
 */

#include "DataManReader.h"

#include "adios2/helper/adiosFunctions.h" //CSVToVector

namespace adios2
{

DataManReader::DataManReader(IO &io, const std::string &name,
                             const OpenMode openMode, MPI_Comm mpiComm)
: Engine("DataManReader", io, name, openMode, mpiComm), m_Man(mpiComm, true)
{
    m_EndMessage = " in call to IO Open DataManReader " + m_Name + "\n";
    Init();
}

void DataManReader::SetCallback(
    std::function<void(const void *, std::string, std::string, std::string,
                       Dims)>
        callback)
{
    m_CallBack = callback;
    m_Man.SetCallback(callback);
}

void DataManReader::Close(const int transportIndex) {}

// PRIVATE
void DataManReader::Init()
{
    auto itRealTime = m_IO.m_Parameters.find("real_time");
    if (itRealTime != m_IO.m_Parameters.end())
    {
        if (itRealTime->second == "yes" || itRealTime->second == "true")
            m_DoRealTime = true;
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
            auto it = m_IO.m_Parameters.find(parameter);
            if (it != m_IO.m_Parameters.end())
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
            auto it = m_IO.m_Parameters.find(parameter);
            if (it != m_IO.m_Parameters.end())
            {
                localVariable = std::stoi(it->second);
            }
        };

        auto is_number = [](const std::string &s) {
            return !s.empty() && std::find_if(s.begin(), s.end(), [](char c) {
                                     return !std::isdigit(c);
                                 }) == s.end();
        };

        /*
        json jmsg;
        for (auto &i : m_IO.m_Parameters)
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
        jmsg["stream_mode"] = "receiver";
        */

        int n_Transports = 1;
        std::vector<Params> para(n_Transports);

        for (unsigned int i = 0; i < para.size(); i++)
        {
            para[i]["type"] = "wan";
            para[i]["transport"] = "zmq";
            para[i]["name"] = "stream";
            para[i]["ipaddress"] = "127.0.0.1";
        }
        m_Man.OpenWANTransports("zmq", adios2::OpenMode::Read, para, true);

        std::string methodType;
        int numChannels = 0;
        lf_AssignString("method_type", methodType);
        lf_AssignInt("num_channels", numChannels);
    }
    else
    {
        InitTransports();
    }
}

} // end namespace adios
