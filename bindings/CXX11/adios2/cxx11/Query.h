/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Query.h : provides type utilities for ADIOS2 C++11 bindings
 *
 *  Created on: Aug 20, 2019
 *      Author: Junmin Gu <jgu@lbl.gov>
 */

#ifndef ADIOS2_BINDINGS_CXX11_QUERY_H_
#define ADIOS2_BINDINGS_CXX11_QUERY_H_

#include <memory> // otherwise MSVC complains about std::shared_ptr

#include "Engine.h"

namespace adios2
{

namespace query
{
class Worker;
} //

class QueryWorker
{
public:
    QueryWorker(const std::string &configFile, adios2::Engine &engine);

    void
    GetResultCoverage(adios2::Box<adios2::Dims> &,
                      std::vector<adios2::Box<adios2::Dims>> &touched_blocks);

private:
    std::shared_ptr<adios2::query::Worker> m_Worker;
}; // class QueryWorker

} // end namespace adios2
#endif /* ADIOS2_BINDINGS_CXX11_QUERY_H_ */
