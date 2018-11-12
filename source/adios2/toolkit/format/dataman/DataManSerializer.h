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
#include "adios2/core/IO.h"
#include "adios2/core/Variable.h"

#include <mutex>
#include <unordered_map>

// A - Address
// C - Count
// D - Data Object ID or File Name
// E - Endian
// G - Global Value
// H - Meatadata Hash
// I - Data Size
// M - Major
// N - Variable Name
// O - Start
// P - Position of Memory Block
// S - Shape
// V - Is Single Value
// X - Index (Used only in deserializer)
// Y - Data Type
// Z - Compression Method
// ZP - Compression Parameters

namespace adios2
{
namespace format
{

class DataManSerializer
{
public:
    DataManSerializer(bool isRowMajor, const bool contiguousMajor,
                      bool isLittleEndian);

    void New(size_t size);

    template <class T>
    void PutVar(const T *inputData, const std::string &varName,
             const Dims &varShape, const Dims &varStart, const Dims &varCount,
             const Dims &varMemStart, const Dims &varMemCount,
             const std::string &doid, const size_t step, const int rank,
             const std::string &address, const Params &params);

    template <class T>
    void PutVar(const core::Variable<T> &variable, const std::string &doid,
                const size_t step, const int rank, const std::string &address,
                const Params &params);

    void PutAttributes(core::IO &io, const int rank);

    const std::shared_ptr<std::vector<char>> GetPack();

#ifdef ADIOS2_HAVE_MPI
    std::shared_ptr<std::vector<char>>
    GetAggregatedMetadata(const MPI_Comm mpiComm);
#endif

    static std::shared_ptr<std::vector<char>> EndSignal(size_t step);

    // from deserializer
    int PutPack(const std::shared_ptr<const std::vector<char>> data);
    template <class T>
    int GetVar(T *output_data, const std::string &varName, const Dims &varStart,
               const Dims &varCount, const size_t step);
    void Erase(const size_t step);
    struct DataManVar
    {
        bool isRowMajor;
        bool isLittleEndian;
        Dims shape;
        Dims count;
        Dims start;
        std::string name;
        std::string doid;
        std::string type;
        size_t step;
        size_t size;
        size_t position;
        size_t index;
        std::string compression;
        Params params;
    };
    std::shared_ptr<const std::vector<DataManVar>>
    GetMetaData(const size_t step);
    const std::unordered_map<size_t, std::shared_ptr<std::vector<DataManVar>>>
    GetMetaData();
    void GetAttributes(core::IO &io);

private:
    template <class T>
    bool PutZfp(nlohmann::json &metaj, size_t &datasize, const T *inputData,
                const Dims &varCount, const Params &params);

    template <class T>
    bool PutSz(nlohmann::json &metaj, size_t &datasize, const T *inputData,
               const Dims &varCount, const Params &params);

    template <class T>
    bool PutBZip2(nlohmann::json &metaj, size_t &datasize, const T *inputData,
                  const Dims &varCount, const Params &params);

    template <class T>
    void PutAttribute(const core::Attribute<T> &attribute, const int rank);

    bool IsCompressionAvailable(const std::string &method,
                                const std::string &type, const Dims &count);

    std::shared_ptr<std::vector<char>> m_Buffer;
    nlohmann::json m_Metadata;
    std::vector<char> m_CompressBuffer;
    size_t m_Position = 0;
    bool m_IsRowMajor;
    bool m_IsLittleEndian;
    bool m_ContiguousMajor;

    // from deserializer
    bool HasOverlap(Dims in_start, Dims in_count, Dims out_start,
                    Dims out_count) const;

    std::unordered_map<size_t, std::shared_ptr<std::vector<DataManVar>>>
        m_MetaDataMap;
    std::unordered_map<int, std::shared_ptr<const std::vector<char>>>
        m_BufferMap;
    size_t m_MaxStep = std::numeric_limits<size_t>::min();
    size_t m_MinStep = std::numeric_limits<size_t>::max();
    nlohmann::json m_GlobalVars;

    std::mutex m_Mutex;
};

} // end namespace format
} // end namespace adios2

#endif
