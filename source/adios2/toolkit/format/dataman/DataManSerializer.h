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
        std::string address;
        std::string compression;
        Params params;
        std::shared_ptr<std::vector<char>> buffer = nullptr;
    };

    // clear and allocate new buffer for writer
    void New(size_t size);

    // put a variable for writer
    template <class T>
    void
    PutVar(const T *inputData, const std::string &varName, const Dims &varShape,
           const Dims &varStart, const Dims &varCount, const Dims &varMemStart,
           const Dims &varMemCount, const std::string &doid, const size_t step,
           const int rank, const std::string &address, const Params &params,
           std::shared_ptr<std::vector<char>> localBuffer = nullptr,
           std::shared_ptr<nlohmann::json> metadataJson = nullptr);

    // another wrapper for PutVar which accepts adios2::core::Variable
    template <class T>
    void PutVar(const core::Variable<T> &variable, const std::string &doid,
                const size_t step, const int rank, const std::string &address,
                const Params &params,
                std::shared_ptr<std::vector<char>> localBuffer = nullptr,
                std::shared_ptr<nlohmann::json> metadataJson = nullptr);

    // put attributes for writer
    void PutAttributes(core::IO &io, const int rank);

    // get the metadata incorporated local buffer for writer, usually called in
    // EndStep
    const std::shared_ptr<std::vector<char>> GetLocalPack();

    // aggregate metadata across all writer ranks and return the aggregated
    // metadata, usually called from master rank of writer
    std::shared_ptr<std::vector<char>>
    GetAggregatedMetadata(const MPI_Comm mpiComm);

    static std::shared_ptr<std::vector<char>> EndSignal(size_t step);

    std::shared_ptr<std::vector<char>> GenerateReply(const std::vector<char> &request, size_t &step);

    int PutPack(const std::shared_ptr<std::vector<char>> data);

    template <class T>
    int GetVar(T *output_data, const std::string &varName, const Dims &varStart,
               const Dims &varCount, const size_t step,
               const Dims &varMemStart = Dims(),
               const Dims &varMemCount = Dims());

    void Erase(const size_t step, const bool allPreviousSteps = false);

    std::shared_ptr<const std::vector<DataManVar>>
    GetMetaData(const size_t step);

    const std::unordered_map<size_t, std::shared_ptr<std::vector<DataManVar>>>
    GetMetaData();

    void GetAttributes(core::IO &io);

    void PutAggregatedMetadata(MPI_Comm mpiComm,
                               std::shared_ptr<std::vector<char>>);

    int PutDeferredRequest(const std::string &variable, const size_t step,
                           const Dims &start, const Dims &count, void *data);
    std::shared_ptr<std::unordered_map<std::string, std::vector<char>>>
    GetDeferredRequest();

    size_t MinStep();
    size_t Steps();

    void ProtectStep(const size_t step);
    void UnprotectStep(const size_t step, const bool allPreviousSteps);

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

    void JsonToDataManVarMap(nlohmann::json &metaJ,
                             std::shared_ptr<std::vector<char>> pack);

    bool CalculateOverlap(const Dims &inStart, const Dims &inCount,
                          const Dims &outStart, const Dims &outCount,
                          Dims &ovlpStart, Dims &ovlpCount);

    std::vector<char> SerializeJson(const nlohmann::json &message);
    nlohmann::json DeserializeJson(const char* start, size_t size);

    void Log(const int level, const std::string &message, const bool mpi = false);

    // local rank single step data and metadata pack buffer, used in writer,
    // only accessed from writer app API thread, does not need mutex
    std::shared_ptr<std::vector<char>> m_LocalBuffer;

    // local rank single step JSON metadata, used in writer, only accessed from
    // writer app API thread, do not need mutex
    nlohmann::json m_MetadataJson;

    // temporary compression buffer, made class member only for saving costs for
    // memory allocation
    std::vector<char> m_CompressBuffer;

    // global aggregated buffer for metadata and data buffer, used in writer
    // (Staging engine) and reader (all engines), needs mutex for accessing
    std::unordered_map<size_t, std::shared_ptr<std::vector<DataManVar>>>
        m_DataManVarMap;
    std::mutex m_DataManVarMapMutex;

    size_t m_CurrentStepBeingRequested;
    std::mutex m_CurrentStepBeingRequestedMutex;

    // for global variables and attributes, needs mutex
    nlohmann::json m_GlobalVars;
    std::mutex m_GlobalVarsMutex;

    // for generating deferred requests, only accessed from reader app thread, does not need mutex
    std::shared_ptr<std::unordered_map<std::string, std::vector<char>>>
        m_DeferredRequestsToSend;

    // string, msgpack, cbor, ubjson
    std::string m_UseJsonSerialization = "string";

    // steps being prevented from erasing, accessed from multiple writer IO threads, needs mutex
    std::vector<size_t> m_ProtectedSteps;
    std::mutex m_ProtectedStepsMutex;

    bool m_IsRowMajor;
    bool m_IsLittleEndian;
    bool m_ContiguousMajor;

    int m_Verbosity = 22;
};

} // end namespace format
} // end namespace adios2

#endif
