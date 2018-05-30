/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataMan.h  classes for DataMan streaming format
 *
 *  Created on: May 11, 2018
 *      Author: Jason Wang
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_DATAMAN_DATAMAN_H_
#define ADIOS2_TOOLKIT_FORMAT_DATAMAN_DATAMAN_H_

#include <nlohmann/json.hpp>

#include "adios2/ADIOSTypes.h"
#include "adios2/core/Variable.h"

namespace adios2
{
namespace format
{

class DataManSerializer
{
public:
    DataManSerializer(size_t size);
    template <class T>
    bool Put(Variable<T> &variable, size_t step, int rank);
    size_t GetBufferSize();
    const std::shared_ptr<std::vector<char>> GetBuffer();

    template <class T>
    static void Put(Variable<T> &variable, size_t step, int rank,
                    std::vector<char> &output);

private:
    std::shared_ptr<std::vector<char>> m_Buffer;
    size_t m_Position;
};

class DataManDeserializer
{
public:
    size_t MaxStep();
    size_t MinStep();
    bool Check(size_t step, std::string variable);
    bool Check(size_t step);
    void Put(std::shared_ptr<std::vector<char>> data);
    template <class T>
    int Get(Variable<T> &variable, size_t step);
    void Erase(size_t step);
    struct DataManVar
    {
        Dims shape;
        Dims count;
        Dims start;
        std::string name;
        std::string type;
        size_t step;
        size_t size;
        size_t position;
        size_t index;
        int rank;
    };
    const std::vector<DataManVar> &GetMetaData(size_t step);

private:
    std::map<size_t, std::vector<DataManVar>> m_MetaDataMap;
    std::vector<std::shared_ptr<std::vector<char>>> m_Buffer;
    std::vector<DataManVar> m_EmptyVector;
    size_t m_MaxStep;
    size_t m_MinStep;
};
}
}

#endif
