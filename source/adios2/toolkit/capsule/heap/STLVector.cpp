/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * STLVector.cpp
 *
 *  Created on: Dec 22, 2016
 *      Author: wfg
 */

#include "STLVector.h"

/// \cond EXCLUDE_FROM_DOXYGEN
#include <new>       //std::bad_alloc
#include <stdexcept> //std::runtime_error
/// \endcond

namespace adios2
{
namespace capsule
{

STLVector::STLVector(const bool debugMode)
: Capsule("heap/STLVector", debugMode)
{
}

char *STLVector::GetData() { return m_Data.data(); }

char *STLVector::GetMetadata() { return m_Metadata.data(); }

size_t STLVector::GetDataSize() const { return m_Data.size(); }

size_t STLVector::GetMetadataSize() const { return m_Metadata.size(); }

void STLVector::ResizeData(const size_t size)
{
    if (m_DebugMode)
    {
        try
        {
            m_Data.resize(size);
        }
        catch (std::bad_alloc &e)
        {
            throw std::runtime_error("ERROR: bad_alloc detected when resizing "
                                     "data buffer with size " +
                                     std::to_string(size) + "\n");
        }
    }
    else
    {
        m_Data.resize(size);
    }
}

void STLVector::ResizeMetadata(const size_t size)
{
    if (m_DebugMode)
    {
        try
        {
            m_Metadata.resize(size);
        }
        catch (std::bad_alloc &e)
        {
            throw std::runtime_error("ERROR: bad_alloc detected when resizing "
                                     "metadata buffer with size " +
                                     std::to_string(size) + "\n");
        }
    }
    else
    {
        m_Metadata.resize(size);
    }
}

} // end namespace capsule
} // end namespace adios
