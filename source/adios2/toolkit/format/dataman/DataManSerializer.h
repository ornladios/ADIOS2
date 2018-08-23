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
    template <class T>
    void Put(const T *inputData, const std::string &varName,
             const Dims &varShape, const Dims &varStart, const Dims &varCount,
             const std::string &doid, const size_t step, const int rank,
             const Params &params, const bool optimizeMetadata);
    template <class T>
    void Put(const core::Variable<T> &variable, const std::string &doid,
             const size_t step, const int rank, const Params &params,
             const bool optimizeMetadata);
    const std::shared_ptr<std::vector<char>> Get();
    float GetMetaRatio();
    static std::shared_ptr<std::vector<char>> EndSignal(size_t step);

private:
    std::shared_ptr<std::vector<char>> m_Buffer;
    std::vector<char> m_CompressBuffer;
    size_t m_Position = 0;
    bool m_IsRowMajor;
    bool m_IsLittleEndian;
    template <class T>
    bool Zfp(nlohmann::json &metaj, size_t &datasize, const T *inputData,
             const Dims &varCount, const Params &params);
    template <class T>
    bool Sz(nlohmann::json &metaj, size_t &datasize, const T *inputData,
            const Dims &varCount, const Params &params);
    template <class T>
    bool BZip2(nlohmann::json &metaj, size_t &datasize, const T *inputData,
               const Dims &varCount, const Params &params);

    bool IsCompressionAvailable(const std::string &method,
                                const std::string &type, const Dims &count);

    size_t m_TotalDataSize;
    size_t m_TotalMetadataSize;

    struct VarDefaults
    {
        std::string doid;
        bool isRowMajor;
        bool isLittleEndian;
        std::string type;
        Dims shape;
    };
    std::map<std::string, VarDefaults> m_VarDefaultsMap;
};

} // end namespace format
} // end namespace adios2

#endif
