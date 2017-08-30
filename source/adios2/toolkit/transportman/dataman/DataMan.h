/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataMan.h
 *
 *  Created on: Jun 1, 2017
 *      Author: Jason Wang wangr1@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_TRANSPORTMAN_DATAMAN_DATAMAN_H_
#define ADIOS2_TOOLKIT_TRANSPORTMAN_DATAMAN_DATAMAN_H_

#include "adios2/toolkit/transportman/TransportMan.h"

#include <json.hpp>

namespace adios2
{
namespace transportman
{

class DataMan : public TransportMan
{

public:
    DataMan(MPI_Comm mpiComm, const bool debugMode);

    virtual ~DataMan() = default;

    void OpenWANTransports(const std::string &name, const Mode openMode,
                           const std::vector<Params> &parametersVector,
                           const bool profile);

private:
    nlohmann::json m_JMessage;

    /** Pick the appropriate default */
    const std::string m_DefaultPort = "22";
};

} // end namespace transportman
} // end namespace adios

#endif /* ADIOS2_TOOLKIT_TRANSPORTMAN_DATAMAN_DATAMAN_H_ */
