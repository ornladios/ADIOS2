/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TableWriter.tcc implementation of template functions with known type
 *
 *  Created on: Apr 6, 2019
 *      Author: Jason Wang w4g@ornl.gov
 */

#ifndef ADIOS2_ENGINE_TABLEWRITER_TCC_
#define ADIOS2_ENGINE_TABLEWRITER_TCC_

#include "TableWriter.h"

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
void TableWriter::PutSyncCommon<std::string>(Variable<std::string> &variable,
                                             const std::string *data)
{
    auto var = m_SubIO.InquireVariable<std::string>(variable.m_Name);
    if (!var)
    {
        var = &m_SubIO.DefineVariable<std::string>(variable.m_Name,
                                                   {LocalValueDim});
    }
    m_SubEngine->Put(*var, data, Mode::Sync);
}

template <>
void TableWriter::PutDeferredCommon<std::string>(
    Variable<std::string> &variable, const std::string *data)
{
    auto var = m_SubIO.InquireVariable<std::string>(variable.m_Name);
    if (!var)
    {
        var = &m_SubIO.DefineVariable<std::string>(variable.m_Name,
                                                   {LocalValueDim});
    }
    m_SubEngine->Put(*var, data, Mode::Deferred);
}

template <class T>
void TableWriter::PutSyncCommon(Variable<T> &variable, const T *data)
{
    PutDeferredCommon(variable, data);
    PerformPuts();
}

template <class T>
void TableWriter::PutDeferredCommon(Variable<T> &variable, const T *data)
{
    auto var = m_SubIO.InquireVariable<T>(variable.m_Name);
    if (!var)
    {
        var = &m_SubIO.DefineVariable<T>(variable.m_Name, variable.m_Shape);
        if (m_UseCompressor == "blosc")
        {
#ifdef ADIOS2_HAVE_BLOSC
            m_Compressor = new compress::CompressBlosc({});
            var->AddOperation(*m_Compressor, {});
#else
            std::cerr
                << "ADIOS2 is not compiled with c-blosc "
                   "(https://github.com/Blosc/c-blosc), compressor not added"
                << std::endl;
#endif
        }
        else if (m_UseCompressor == "bzip2")
        {
#ifdef ADIOS2_HAVE_BZIP2
            m_Compressor = new compress::CompressBZIP2({});
            var->AddOperation(*m_Compressor, {});
#else
            std::cerr << "ADIOS2 is not compiled with Bzip2 "
                         "(https://gitlab.com/federicomenaquintero/bzip2), "
                         "compressor not added"
                      << std::endl;
#endif
        }
        else if (m_UseCompressor == "zfp")
        {
#ifdef ADIOS2_HAVE_ZFP
            if (var->m_Type == helper::GetDataType<float>() ||
                var->m_Type == helper::GetDataType<double>() ||
                var->m_Type == helper::GetDataType<std::complex<float>>() ||
                var->m_Type == helper::GetDataType<std::complex<double>>())
            {
                if (m_UseAccuracy.empty())
                {
                    std::cerr << "Parameter accuracy for lossy compression is "
                                 "not specified, compressor not added"
                              << std::endl;
                }
                else
                {
                    m_Compressor = new compress::CompressZFP({});
                    var->AddOperation(*m_Compressor, {{ops::zfp::key::accuracy,
                                                       m_UseAccuracy}});
                }
            }
#else
            std::cerr << "ADIOS2 is not compiled with ZFP "
                         "(https://github.com/LLNL/zfp), "
                         "compressor not added"
                      << std::endl;
#endif
        }
        else if (m_UseCompressor == "sz")
        {
#ifdef ADIOS2_HAVE_SZ
            if (var->m_Type == helper::GetDataType<float>() ||
                var->m_Type == helper::GetDataType<double>() ||
                var->m_Type == helper::GetDataType<std::complex<float>>() ||
                var->m_Type == helper::GetDataType<std::complex<double>>())
            {
                if (m_UseAccuracy.empty())
                {
                    std::cerr << "Parameter accuracy for lossy compression is "
                                 "not specified, compressor not added"
                              << std::endl;
                }
                else
                {
                    m_Compressor = new compress::CompressSZ({});
                    var->AddOperation(*m_Compressor, {{ops::sz::key::accuracy,
                                                       m_UseAccuracy}});
                }
            }
#else
            std::cerr << "ADIOS2 is not compiled with SZ "
                         "(https://github.com/szcompressor/SZ), "
                         "compressor not added"
                      << std::endl;
#endif
        }
    }

    if (m_IndexerMap[variable.m_Name])
    {
        // TODO: implement indexing
    }
    else
    {
        var->SetSelection({variable.m_Start, variable.m_Count});
        m_SubEngine->Put(*var, data, Mode::Deferred);
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_TABLEWRITER_TCC_ */
