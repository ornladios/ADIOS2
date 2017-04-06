/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Method.cpp
 *
 *  Created on: Jan 6, 2017
 *      Author: wfg
 */

#include <utility>

#include "core/Method.h"
#include "functions/adiosFunctions.h"

namespace adios
{

Method::Method(const std::string name, const bool debugMode)
: m_Name{name}, m_DebugMode{debugMode}
{
}

bool Method::IsUserDefined()
{
    return false; // TODO(wfg): check if XML has the method defined
}

void Method::SetEngine(const std::string type) { m_Type = type; }

void Method::AllowThreads(const unsigned int nThreads)
{
    if (nThreads > 1)
    {
        m_nThreads = nThreads;
    }
    else
    {
        m_nThreads = 1;
    }
}

// PRIVATE Functions
void Method::AddTransportParameters(const std::string type,
                                    const std::vector<std::string> &parameters)
{
    if (m_DebugMode == true)
    {
        if (type.empty() || type.find("=") != type.npos)
        {
            throw std::invalid_argument(
                "ERROR: first argument in AddTransport must "
                "be a single word for transport\n");
        }
    }

    std::map<std::string, std::string> mapParameters =
        BuildParametersMap(parameters, m_DebugMode);
    if (m_DebugMode == true)
    {
        if (mapParameters.count("transport") == 1)
        {
            std::invalid_argument("ERROR: transport can't be redefined with "
                                  "\"transport=type\", "
                                  "type must be the first argument\n");
        }
    }

    mapParameters["transport"] = type;
    m_TransportParameters.push_back(mapParameters);
}

} // end namespace adios
