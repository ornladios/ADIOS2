/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Heap.h
 *
 *  Created on: Dec 19, 2016
 *      Author: wfg
 */

#ifndef ADIOS2_CAPSULE_HEAP_STLVECTOR_H_
#define ADIOS2_CAPSULE_HEAP_STLVECTOR_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <vector>
/// \endcond

#include "adios2/ADIOSConfig.h"
#include "adios2/core/Capsule.h"

namespace adios
{
namespace capsule
{

/**
 * Data and Metadata buffers are allocated in the Heap
 */
class STLVector : public Capsule
{

public:
    /** data buffer allocated using the STL in heap */
    std::vector<char> m_Data;
    /** might be used in cases other than files */
    std::vector<char> m_Metadata;

    /**
     * Unique constructor
     * @param debugMode true: exceptions checks
     */
    STLVector(const bool debugMode = false);

    ~STLVector() = default;

    char *GetData();
    char *GetMetadata();

    size_t GetDataSize() const;
    size_t GetMetadataSize() const;

    void ResizeData(const size_t size);
    void ResizeMetadata(const size_t size);
};

} // end namespace capsule
} // end namespace

#endif /* ADIOS2_CAPSULE_HEAP_STLVECTOR_H_ */
