/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataManDeserializer.h Deserializer class for DataMan streaming format
 *
 *  Created on: May 11, 2018
 *      Author: Jason Wang
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_DATAMAN_DATAMANDESERIALIZER_H_
#define ADIOS2_TOOLKIT_FORMAT_DATAMAN_DATAMANDESERIALIZER_H_

#include <nlohmann/json.hpp>

#include "adios2/ADIOSTypes.h"
#include "adios2/core/IO.h"
#include "adios2/core/Variable.h"

#include <mutex>
#include <unordered_map>

namespace adios2
{
namespace format
{

class DataManDeserializer
{
public:
    DataManDeserializer(const bool isRowMajor, const bool isLittleEndian);
    size_t MaxStep();
    size_t MinStep();
    int Put(const std::shared_ptr<const std::vector<char>> data);
    template <class T>
    int Get(T *output_data, const std::string &varName, const Dims &varStart,
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
    bool HasOverlap(Dims in_start, Dims in_count, Dims out_start,
                    Dims out_count) const;

    std::unordered_map<size_t, std::shared_ptr<std::vector<DataManVar>>>
        m_MetaDataMap;
    std::unordered_map<int, std::shared_ptr<const std::vector<char>>>
        m_BufferMap;
    size_t m_MaxStep = std::numeric_limits<size_t>::min();
    size_t m_MinStep = std::numeric_limits<size_t>::max();
    bool m_IsRowMajor;
    bool m_IsLittleEndian;
    nlohmann::json m_GlobalVars;

    std::mutex m_Mutex;
};

} // end namespace format
} // end namespace adios2

#endif
