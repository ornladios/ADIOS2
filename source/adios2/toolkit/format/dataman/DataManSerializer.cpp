/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataManSerializer.cpp Serializer class for DataMan streaming format
 *
 *  Created on: May 11, 2018
 *      Author: Jason Wang
 */

#include "DataManSerializer.tcc"

#include <cstring>
#include <iostream>

namespace adios2
{
namespace format
{

DataManSerializer::DataManSerializer(bool isRowMajor, bool isLittleEndian)
{
    m_IsRowMajor = isRowMajor;
    m_IsLittleEndian = isLittleEndian;
}

void DataManSerializer::New(size_t size)
{
    // make a new shared object each time because the old shared object could
    // still be alive and needed somewhere in the workflow, for example the
    // queue in transport manager. It will be automatically released when the
    // entire workflow finishes using it.
    m_Buffer = std::make_shared<std::vector<char>>();
    m_Buffer->reserve(size);
    m_Position = 0;
}

const std::shared_ptr<std::vector<char>> DataManSerializer::Get()
{
    return m_Buffer;
}

} // namespace format
} // namespace adios2
