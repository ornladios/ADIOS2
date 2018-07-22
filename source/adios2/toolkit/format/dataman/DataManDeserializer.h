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
    DataManDeserializer(bool isRowMajor, bool isLittleEndian);
    size_t MaxStep();
    size_t MinStep();
    void Put(std::shared_ptr<std::vector<char>> data);
    template <class T>
    int Get(core::Variable<T> &variable, size_t step);
    void Erase(size_t step);
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
        int rank;
        std::string compression;
        float compressionRate;
    };
    bool GetVarList(size_t step, std::vector<DataManVar> &varList);
    const std::shared_ptr<std::vector<DataManVar>> GetMetaData(size_t step);

private:
    bool HasOverlap(Dims in_start, Dims in_count, Dims out_start,
                    Dims out_count);
    bool BufferContainsSteps(int index, size_t begin, size_t end);

    std::unordered_map<size_t, std::shared_ptr<std::vector<DataManVar>>>
        m_MetaDataMap;
    std::unordered_map<int, std::shared_ptr<std::vector<char>>> m_BufferMap;
    size_t m_MaxStep = std::numeric_limits<size_t>::min();
    size_t m_MinStep = std::numeric_limits<size_t>::max();
    bool m_IsRowMajor;
    bool m_IsLittleEndian;

    std::mutex m_MutexMetaData;
    std::mutex m_MutexBuffer;
    std::mutex m_MutexMaxMin;
};

} // end namespace format
} // end namespace adios2

#endif
