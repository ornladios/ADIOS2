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
#include "DataManWriter.tcc"

#include <iostream> //needs to go away, this is just for demo purposes

#include "adios2/helper/adiosFunctions.h" //CSVToVector

namespace adios2
{

DataManWriter::DataManWriter(IO &io, const std::string &name,
                             const Mode openMode, MPI_Comm mpiComm)
: Engine("DataManWriter", io, name, openMode, mpiComm)
{
    m_EndMessage = ", in call to Open DataManWriter\n";
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

void DataManWriter::Advance(const float timeoutSeconds) { m_Man.flush(); }

void DataManWriter::Close(const int transportIndex) { m_Man.flush(); }

// PRIVATE functions below
void DataManWriter::Init()
{
    auto lf_SetBoolParameter = [&](const std::string key, bool &parameter) {

        auto itKey = m_IO.m_Parameters.find(key);
        if (itKey != m_IO.m_Parameters.end())
        {
            if (itKey->second == "yes" || itKey->second == "true")
            {
                parameter = true;
            }
            else if (itKey->second == "no" || itKey->second == "false")
            {
                parameter = false;
            }
        }
    };

    lf_SetBoolParameter("real_time", m_DoRealTime);
    lf_SetBoolParameter("monitoring", m_DoMonitor);

    if (m_DoRealTime)
    {
        /**
         * Lambda function that assigns a parameter in m_Method to a
         * localVariable
         * of type std::string
         */
        auto lf_AssignString = [&](const std::string parameter,
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
        auto lf_AssignInt = [&](const std::string parameter,
                                int &localVariable) {
            auto it = m_IO.m_Parameters.find(parameter);
            if (it != m_IO.m_Parameters.end())
            {
                localVariable = std::stoi(it->second);
            }
        };

        auto lf_IsNumber = [](const std::string &s) {
            return !s.empty() && std::find_if(s.begin(), s.end(), [](char c) {
                                     return !std::isdigit(c);
                                 }) == s.end();
        };

        json jmsg;
        for (const auto &i : m_IO.m_Parameters)
        {
            if (lf_IsNumber(i.second))
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
        lf_AssignString("method_type", method_type);

        int num_channels = 0;
        lf_AssignInt("num_channels", num_channels);
    }
}

#define declare_type(T)                                                        \
    void DataManWriter::DoWrite(Variable<T> &variable, const T *values)        \
    {                                                                          \
        DoWriteCommon(variable, values);                                       \
    }
ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

} // end namespace adios
