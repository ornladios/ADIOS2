/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * StagingMan.h
 *
 *  Created on: Oct 1, 2018
 *      Author: Jason Wang wangr1@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_TRANSPORTMAN_STAGINGMAN_H_
#define ADIOS2_TOOLKIT_TRANSPORTMAN_STAGINGMAN_H_

#include <queue>
#include <thread>

#include "adios2/core/IO.h"
#include "adios2/core/Operator.h"
#include "adios2/toolkit/format/bp3/BP3.h"
#include "adios2/toolkit/transportman/TransportMan.h"

namespace adios2
{
namespace transportman
{

class StagingMan
{

public:
    StagingMan(MPI_Comm mpiComm, const bool debugMode);

    ~StagingMan();

    void OpenTransports(const std::vector<Params> &paramsVector,
                        const Mode openMode, const bool profile);

    void Request(const std::vector<char> &request,
                 std::shared_ptr<std::vector<char>> reply,
                 const std::string &address);

private:
    bool GetBoolParameter(const Params &params, const std::string key);
    bool GetStringParameter(const Params &params, const std::string key,
                            std::string &value);
    bool GetIntParameter(const Params &params, const std::string key,
                         int &value);

    int m_Timeout = 10;

    std::string m_Address;
};

} // end namespace transportman
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_TRANSPORTMAN_STAGINGMAN_H_ */
