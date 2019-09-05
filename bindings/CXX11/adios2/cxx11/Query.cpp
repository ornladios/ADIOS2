#include "Query.h"
#include "adios2/toolkit/query/Worker.h"

namespace adios2
{
QueryWorker::QueryWorker(const std::string &configFile, adios2::Engine &reader)
{
    adios2::query::Worker *m =
        adios2::query::GetWorker(configFile, reader.m_Engine);
    if (m == nullptr)
        throw std::invalid_argument("ERROR: unable to construct query. ");
    m_Worker = std::make_shared<adios2::query::Worker>(*m);
}

void QueryWorker::GetResultCoverage(
    adios2::Box<adios2::Dims> &outputSelection,
    std::vector<adios2::Box<adios2::Dims>> &touched_blocks)
{
    return m_Worker->GetResultCoverage(outputSelection, touched_blocks);
}
}
