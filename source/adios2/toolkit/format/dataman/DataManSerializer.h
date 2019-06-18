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

#include "adios2/common/ADIOSTypes.h"
#include "adios2/core/IO.h"
#include "adios2/toolkit/profiling/taustubs/tautimer.hpp"

#include <mutex>
#include <unordered_map>

#include <nlohmann/json.hpp>

// A - Address
// C - Count
// D - Data Object ID or File Name
// E - Endian
// G - Global Value, for attributes
// H - Meatadata Hash
// I - Data Size
// M - Major
// N - Variable Name
// O - Start
// P - Position of Memory Block
// S - Shape
// T - Step
// V - Is Single Value, for attributes
// Y - Data Type
// Z - Compression Method
// ZP - Compression Parameters
// - - Min
// + - Max
// # - Value

namespace adios2
{
namespace format
{

using VecPtr = std::shared_ptr<std::vector<char>>;
using VecPtrMap = std::unordered_map<int64_t, VecPtr>;
using JsonPtr = std::shared_ptr<nlohmann::json>;

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
    std::vector<char> min;
    std::vector<char> max;
    std::vector<char> value;
    size_t step;
    size_t size;
    size_t position;
    int rank;
    std::string address;
    std::string compression;
    Params params;
    VecPtr buffer = nullptr;
};

using DmvVecPtr = std::shared_ptr<std::vector<DataManVar>>;
using DmvVecPtrMap = std::unordered_map<size_t, DmvVecPtr>;
using DmvVecPtrMapPtr = std::shared_ptr<DmvVecPtrMap>;
using DeferredRequestMap =
    std::unordered_map<std::string, std::shared_ptr<std::vector<char>>>;
using DeferredRequestMapPtr = std::shared_ptr<DeferredRequestMap>;

class DataManSerializer
{
public:
    DataManSerializer(bool isRowMajor, const bool contiguousMajor,
                      bool isLittleEndian, MPI_Comm mpiComm);

    // clear and allocate new buffer for writer
    void New(size_t size);

    // put a variable for writer
    template <class T>
    void
    PutVar(const T *inputData, const std::string &varName, const Dims &varShape,
           const Dims &varStart, const Dims &varCount, const Dims &varMemStart,
           const Dims &varMemCount, const std::string &doid, const size_t step,
           const int rank, const std::string &address, const Params &params,
           VecPtr localBuffer = nullptr, JsonPtr metadataJson = nullptr);

    // another wrapper for PutVar which accepts adios2::core::Variable
    template <class T>
    void PutVar(const core::Variable<T> &variable, const std::string &doid,
                const size_t step, const int rank, const std::string &address,
                const Params &params, VecPtr localBuffer = nullptr,
                JsonPtr metadataJson = nullptr);

    // read attributes from IO and put into m_StaticDataJson
    void PutAttributes(core::IO &io);

    // read attributes form m_StaticDataJson and put into IO
    void GetAttributes(core::IO &io);

    // attach m_StaticDataJson to m_MetadataJson
    void AttachAttributes();

    // get the metadata incorporated local buffer for writer, usually called in
    // EndStep
    VecPtr GetLocalPack();

    // aggregate metadata across all writer ranks and put it into map
    void AggregateMetadata();

    VecPtr GetAggregatedMetadataPack(const int64_t stepRequested,
                                     int64_t &stepProvided,
                                     const int64_t appID);

    static VecPtr EndSignal(size_t step);

    VecPtr GenerateReply(
        const std::vector<char> &request, size_t &step,
        const std::unordered_map<std::string, Params> &compressionParams);

    int PutPack(const VecPtr data);

    template <class T>
    int GetVar(T *output_data, const std::string &varName, const Dims &varStart,
               const Dims &varCount, const size_t step,
               const Dims &varMemStart = Dims(),
               const Dims &varMemCount = Dims());

    void Erase(const size_t step, const bool allPreviousSteps = false);

    bool IsStepProtected(const int64_t step);

    DmvVecPtr GetMetaData(const size_t step);

    const DmvVecPtrMap GetMetaData();

    void PutAggregatedMetadata(VecPtr input, MPI_Comm mpiComm);

    int PutDeferredRequest(const std::string &variable, const size_t step,
                           const Dims &start, const Dims &count, void *data);
    DeferredRequestMapPtr GetDeferredRequest();

    size_t MinStep();
    size_t Steps();

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

    void ProtectStep(const int64_t step, const int64_t id);

    template <class T>
    void PutAttribute(const core::Attribute<T> &attribute);

    bool IsCompressionAvailable(const std::string &method,
                                const std::string &type, const Dims &count);

    void JsonToDataManVarMap(nlohmann::json &metaJ, VecPtr pack);

    bool CalculateOverlap(const Dims &inStart, const Dims &inCount,
                          const Dims &outStart, const Dims &outCount,
                          Dims &ovlpStart, Dims &ovlpCount);

    VecPtr SerializeJson(const nlohmann::json &message);
    nlohmann::json DeserializeJson(const char *start, size_t size);

    template <typename T>
    void CalculateMinMax(const T *data, const Dims &count,
                         nlohmann::json &metaj);

    void Log(const int level, const std::string &message, const bool mpi,
             const bool endline);

    // local rank single step data and metadata pack buffer, used in writer,
    // only accessed from writer app API thread, does not need mutex
    VecPtr m_LocalBuffer;

    // local rank single step JSON metadata, used in writer, only accessed from
    // writer app API thread, do not need mutex
    nlohmann::json m_MetadataJson;

    // temporary compression buffer, made class member only for saving costs for
    // memory allocation
    std::vector<char> m_CompressBuffer;

    // global aggregated buffer for metadata and data buffer, used in writer
    // (Staging engine) and reader (all engines), needs mutex for accessing
    DmvVecPtrMap m_DataManVarMap;
    std::mutex m_DataManVarMapMutex;

    std::unordered_map<size_t, std::vector<size_t>> m_ProtectedStepsToAggregate;
    std::unordered_map<size_t, std::vector<size_t>> m_ProtectedStepsAggregated;
    std::mutex m_ProtectedStepsMutex;

    // Aggregated metadata json, used in writer, accessed from API thread and
    // reply thread, needs mutex
    nlohmann::json m_AggregatedMetadataJson;
    std::mutex m_AggregatedMetadataJsonMutex;

    // for global variables and attributes, needs mutex
    nlohmann::json m_StaticDataJson;
    std::mutex m_StaticDataJsonMutex;
    bool m_StaticDataFinished = false;

    // for generating deferred requests, only accessed from reader app thread,
    // does not need mutex

    DeferredRequestMapPtr m_DeferredRequestsToSend;

    // string, msgpack, cbor, ubjson
    std::string m_UseJsonSerialization = "string";

    bool m_IsRowMajor;
    bool m_IsLittleEndian;
    bool m_ContiguousMajor;
    bool m_EnableStat = true;
    int m_MpiRank;
    int m_MpiSize;
    MPI_Comm m_MpiComm;

    int m_Verbosity = 0;
};

} // end namespace format
} // end namespace adios2

#endif
