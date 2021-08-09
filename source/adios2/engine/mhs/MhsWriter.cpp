/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * MhsWriter.cpp
 *
 *  Created on: Apr 6, 2019
 *      Author: Jason Wang w4g@ornl.gov
 */

#include "MhsWriter.tcc"
#include "adios2/helper/adiosFunctions.h"
#include "adios2/operator/compress/CompressSirius.h"
#ifdef ADIOS2_HAVE_BLOSC
#include "adios2/operator/compress/CompressBlosc.h"
#endif
#ifdef ADIOS2_HAVE_BZIP2
#include "adios2/operator/compress/CompressBZIP2.h"
#endif
#ifdef ADIOS2_HAVE_ZFP
#include "adios2/operator/compress/CompressZFP.h"
#endif
#ifdef ADIOS2_HAVE_SZ
#include "adios2/operator/compress/CompressSZ.h"
#endif

namespace adios2
{
namespace core
{
namespace engine
{

MhsWriter::MhsWriter(IO &io, const std::string &name, const Mode mode,
                     helper::Comm comm)
: Engine("MhsWriter", io, name, mode, std::move(comm))
{
    helper::GetParameter(io.m_Parameters, "tiers", m_Tiers);
    for (const auto &transportParams : io.m_TransportsParameters)
    {
        auto itVar = transportParams.find("variable");
        if (itVar == transportParams.end())
        {
            continue;
        }
        auto itCompressor = transportParams.find("transport");
        if (itCompressor == transportParams.end())
        {
            continue;
        }

        Params params;

        if (itCompressor->second == "blosc")
        {
#ifdef ADIOS2_HAVE_BLOSC
            m_OperatorMap.emplace(itVar->second,
                                  new compress::CompressBlosc(params));
#else
            std::cerr
                << "ADIOS2 is not compiled with c-blosc "
                   "(https://github.com/Blosc/c-blosc), compressor not added"
                << std::endl;
#endif
        }
        else if (itCompressor->second == "bzip2")
        {
#ifdef ADIOS2_HAVE_BZIP2
            m_OperatorMap.emplace(itVar->second,
                                  new compress::CompressBZIP2(params));
#else
            std::cerr << "ADIOS2 is not compiled with Bzip2 "
                         "(https://gitlab.com/federicomenaquintero/bzip2), "
                         "compressor not added"
                      << std::endl;
#endif
        }
        else if (itCompressor->second == "zfp")
        {
#ifdef ADIOS2_HAVE_ZFP
            m_OperatorMap.emplace(itVar->second,
                                  new compress::CompressZFP(params));
#else
            std::cerr << "ADIOS2 is not compiled with ZFP "
                         "(https://github.com/LLNL/zfp), "
                         "compressor not added"
                      << std::endl;
#endif
        }
        else if (itCompressor->second == "sz")
        {
#ifdef ADIOS2_HAVE_SZ
            m_OperatorMap.emplace(itVar->second,
                                  new compress::CompressSZ(params));
#else
            std::cerr << "ADIOS2 is not compiled with SZ "
                         "(https://github.com/szcompressor/SZ), "
                         "compressor not added"
                      << std::endl;
#endif
        }
        else if (itCompressor->second == "sirius")
        {
            params.emplace("tiers", std::to_string(m_Tiers));
            m_OperatorMap.emplace(itVar->second,
                                  new compress::CompressSirius(params));
        }
        else
        {
            throw("invalid operator");
        }
    }
    if (m_Tiers > 1)
    {
        for (int i = 0; i < m_Tiers; ++i)
        {
            m_SubIOs.emplace_back(
                &io.m_ADIOS.DeclareIO("SubIO" + std::to_string(i)));
            m_SubEngines.emplace_back(&m_SubIOs.back()->Open(
                m_Name + ".tier" + std::to_string(i), adios2::Mode::Write));
        }
    }
    else
    {
        m_SubIOs.emplace_back(&io.m_ADIOS.DeclareIO("SubIO"));
        m_SubEngines.emplace_back(
            &m_SubIOs.back()->Open(m_Name, adios2::Mode::Write));
    }
}

MhsWriter::~MhsWriter() {}

StepStatus MhsWriter::BeginStep(StepMode mode, const float timeoutSeconds)
{
    for (auto &e : m_SubEngines)
    {
        e->BeginStep(mode, timeoutSeconds);
    }
    return StepStatus::OK;
}

size_t MhsWriter::CurrentStep() const { return m_SubEngines[0]->CurrentStep(); }

void MhsWriter::PerformPuts()
{
    for (auto &e : m_SubEngines)
    {
        e->PerformPuts();
    }
}

void MhsWriter::EndStep()
{
    for (auto &e : m_SubEngines)
    {
        e->EndStep();
    }
}

void MhsWriter::Flush(const int transportIndex)
{
    for (auto &e : m_SubEngines)
    {
        e->Flush(transportIndex);
    }
}

// PRIVATE

#define declare_type(T)                                                        \
    void MhsWriter::DoPutSync(Variable<T> &variable, const T *data)            \
    {                                                                          \
        PutSyncCommon(variable, data);                                         \
    }                                                                          \
    void MhsWriter::DoPutDeferred(Variable<T> &variable, const T *data)        \
    {                                                                          \
        PutDeferredCommon(variable, data);                                     \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void MhsWriter::DoClose(const int transportIndex)
{
    for (auto &e : m_SubEngines)
    {
        e->Close();
        e = nullptr;
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
