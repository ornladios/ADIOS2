/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataManSerializer.h Serializer class for DataMan streaming format
 *
 *  Created on: May 11, 2018
 *      Author: Jason Wang
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_DATAMAN_DATAMANSERIALIZER_H_
#define ADIOS2_TOOLKIT_FORMAT_DATAMAN_DATAMANSERIALIZER_H_

#include <nlohmann/json.hpp>

#include "adios2/ADIOSTypes.h"
#include "adios2/core/Variable.h"

#include <mutex>
#include <unordered_map>

namespace adios2
{
namespace format
{

class DataManSerializer
{
public:
    DataManSerializer(bool isRowMajor, bool isLittleEndian);
    void New(size_t size);
    const std::shared_ptr<std::vector<char>> Get();

    template <class T>
    bool Put(core::Variable<T> &variable, std::string doid, size_t step,
             int rank, const Params &params);

    template <class T>
    bool PutRaw(core::Variable<T> &variable, std::string doid, size_t step,
                int rank, const Params &params);

#ifdef ADIOS2_HAVE_ZFP
    template <class T>
    bool PutZfp(core::Variable<T> &variable, std::string doid, size_t step,
                int rank, const Params &params);
#endif

private:
    std::shared_ptr<std::vector<char>> m_Buffer;
    std::vector<char> m_CompressBuffer;
    size_t m_Position = 0;
    bool m_IsRowMajor;
    bool m_IsLittleEndian;
};

} // end namespace format
} // end namespace adios2

#endif
