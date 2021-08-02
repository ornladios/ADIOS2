/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * MhsWriter.tcc implementation of template functions with known type
 *
 *  Created on: Apr 6, 2019
 *      Author: Jason Wang w4g@ornl.gov
 */

#ifndef ADIOS2_ENGINE_MHSWRITER_TCC_
#define ADIOS2_ENGINE_MHSWRITER_TCC_

#include "MhsWriter.h"
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

template <>
void MhsWriter::PutSyncCommon<std::string>(Variable<std::string> &variable,
                                           const std::string *data)
{
    auto var = m_SubIO.InquireVariable<std::string>(variable.m_Name);
    if (!var)
    {
        var = &m_SubIO.DefineVariable<std::string>(variable.m_Name,
                                                   {LocalValueDim});
    }
    for (auto &e : m_SubEngines)
    {
        e->Put(*var, data, Mode::Sync);
    }
}

template <>
void MhsWriter::PutDeferredCommon<std::string>(Variable<std::string> &variable,
                                               const std::string *data)
{
    auto var = m_SubIO.InquireVariable<std::string>(variable.m_Name);
    if (!var)
    {
        var = &m_SubIO.DefineVariable<std::string>(variable.m_Name,
                                                   {LocalValueDim});
    }
    for (auto &e : m_SubEngines)
    {
        e->Put(*var, data, Mode::Deferred);
    }
}

template <class T>
void MhsWriter::PutSyncCommon(Variable<T> &variable, const T *data)
{
    PutDeferredCommon(variable, data);
    PerformPuts();
}

template <class T>
void MhsWriter::PutDeferredCommon(Variable<T> &variable, const T *data)
{
    auto var = m_SubIO.InquireVariable<T>(variable.m_Name);
    if (!var)
    {
        var = &m_SubIO.DefineVariable<T>(variable.m_Name, variable.m_Shape);
        auto it = m_OperatorMap.find(variable.m_Name);
        if (it != m_OperatorMap.end())
        {
            auto itCompressor = it->second.find("transport");
            if (itCompressor == it->second.end())
            {
                throw("compressor not specified");
            }
            if (itCompressor->second == "blosc")
            {
#ifdef ADIOS2_HAVE_BLOSC
                auto compressor = new compress::CompressBlosc({});
                m_Compressors.push_back(compressor);
                var->AddOperation(*compressor, {});
#else
                std::cerr << "ADIOS2 is not compiled with c-blosc "
                             "(https://github.com/Blosc/c-blosc), compressor "
                             "not added"
                          << std::endl;
#endif
            }
            else if (itCompressor->second == "bzip2")
            {
#ifdef ADIOS2_HAVE_BZIP2
                auto compressor = new compress::CompressBZIP2({});
                m_Compressors.push_back(compressor);
                var->AddOperation(*compressor, {});
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
                auto itAccuracy = it->second.find("accuracy");
                if (itAccuracy != it->second.end())
                {
                    auto compressor = new compress::CompressZFP({});
                    m_Compressors.push_back(compressor);
                    var->AddOperation(*compressor, {{ops::zfp::key::accuracy,
                                                     itAccuracy->second}});
                }
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
                auto itAccuracy = it->second.find("accuracy");
                if (itAccuracy != it->second.end())
                {
                    auto compressor = new compress::CompressSZ({});
                    m_Compressors.push_back(compressor);
                    var->AddOperation(*compressor, {{ops::sz::key::accuracy,
                                                     itAccuracy->second}});
                }
#else
                std::cerr << "ADIOS2 is not compiled with SZ "
                             "(https://github.com/szcompressor/SZ), "
                             "compressor not added"
                          << std::endl;
#endif
            }
            else if (itCompressor->second == "sirius")
            {
                auto compressor =
                    new compress::CompressSirius(m_IO.m_Parameters);
                m_Compressors.push_back(compressor);
                var->AddOperation(*compressor, {});
            }
            else
            {
                throw("invalid operator");
            }
        }
    }

    var->SetSelection({variable.m_Start, variable.m_Count});
    for (auto &e : m_SubEngines)
    {
        e->Put(*var, data, Mode::Deferred);
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_TABLEWRITER_TCC_ */
