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
// T - Step
// V - Is Single Value
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
        int rank;
        std::string compression;
        Params params;
        std::shared_ptr<std::vector<char>> buffer = nullptr;
    };


    // Serializer functions

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

    const std::shared_ptr<std::vector<char>> GetLocalPack();

    std::shared_ptr<std::vector<char>>
    GetAggregatedMetadata(const MPI_Comm mpiComm);

    static std::shared_ptr<std::vector<char>> EndSignal(size_t step);

    // Deserializer functions

    int PutPack(const std::shared_ptr<std::vector<char>> data);

    template <class T>
    int GetVar(T *output_data, const std::string &varName, const Dims &varStart,
               const Dims &varCount, const size_t step);

    void Erase(const size_t step);

    std::shared_ptr<const std::vector<DataManVar>>
    GetMetaData(const size_t step);

    const std::unordered_map<size_t, std::shared_ptr<std::vector<DataManVar>>>
    GetMetaData();

    void GetAttributes(core::IO &io);

    void PutAggregatedMetadata(MPI_Comm mpiComm,
                               std::shared_ptr<std::vector<char>>);

    int PutDeferredRequest(const std::string &variable, const size_t step, const Dims &start, const Dims &count, void* data);
    std::shared_ptr<std::unordered_map<int, std::vector<char>>> GetDeferredRequest();
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

    void JsonToDataManVarMap(nlohmann::json &metaJ, std::shared_ptr<std::vector<char>> pack);

    // local rank single step data and metadata pack buffer, used in writer, only accessed from writer app API thread, does not need mutex
    std::shared_ptr<std::vector<char>> m_LocalBuffer;

    // local rank single step JSON metadata, used in writer, only accessed from writer app API thread, do not need mutex
    nlohmann::json m_MetadataJson;

    // temporary compression buffer, made class member only for saving costs for memory allocation
    std::vector<char> m_CompressBuffer;
<<<<<<< HEAD
    size_t m_Position = 0;
    bool m_IsRowMajor;
    bool m_IsLittleEndian;
    bool m_ContiguousMajor;
=======
>>>>>>> added deferred requests handling in staging engine

    // global aggregated buffer for metadata and data buffer, used in writer and reader, needs mutex for accessing
    std::unordered_map<size_t, std::shared_ptr<std::vector<DataManVar>>> m_DataManVarMap;

    // for global variables and attributes, needs mutex
    nlohmann::json m_GlobalVars;

    struct Request
    {
        std::string variable;
        size_t step;
        Dims start;
        Dims count;
        void *data;
    };

    std::vector<Request> m_DeferredRequests;
    std::shared_ptr<std::unordered_map<int, std::vector<char>>> m_DeferredRequestsToSend;

    std::mutex m_Mutex;
    bool m_IsRowMajor;
    bool m_IsLittleEndian;

};

} // end namespace format
} // end namespace adios2

#endif
