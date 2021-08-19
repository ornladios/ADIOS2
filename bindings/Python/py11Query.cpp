/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * py11Query.h :
 *
 *  Created on: August 5, 2021
 *      Author: Junmin Gu (jgu@lbl.gov)
 */

#include "py11Query.h"

namespace adios2
{
namespace py11
{

Query::Query(adios2::query::Worker *qw) : m_QueryWorker(qw) {}

Query::Query(std::string queryFile, Engine reader)
{
    adios2::query::Worker *m =
        adios2::query::GetWorker(queryFile, reader.m_Engine);
    if (m == nullptr)
        throw std::invalid_argument("ERROR: unable to construct query. ");
    m_QueryWorker = std::make_shared<adios2::query::Worker>(std::move(*m));
    delete m;
}

Query::operator bool() const noexcept
{
    return (m_QueryWorker == nullptr) ? false : true;
}

std::vector<Box<Dims>> Query::GetResult()
{
    // std::cout<<"Do something"<<std::endl;
    adios2::Box<adios2::Dims> empty; // look into all data
    std::vector<Box<Dims>> touched_blocks;
    m_QueryWorker->GetResultCoverage(empty, touched_blocks);
    return touched_blocks;
}

} // py11
} // adios2
