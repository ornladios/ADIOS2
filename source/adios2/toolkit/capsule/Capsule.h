/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Capsule.h
 *
 *  Created on: Dec 7, 2016
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_CAPSULE_CAPSULE_H_
#define ADIOS2_TOOLKIT_CAPSULE_CAPSULE_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <string>
/// \endcond

#include "adios2/ADIOSConfig.h"
#include "adios2/ADIOSTypes.h"

namespace adios2
{

/**
 * Base class that raw data and metadata buffers, used by Engine.
 * Derived classes will allocate their own buffer in different memory spaces.
 * e.g. locally (heap) or in shared memory (virtual memory)
 */
class Capsule
{

public:
    /** Derived class ID */
    const std::string m_Type;

    /** position in current data buffer */
    size_t m_DataPosition = 0;

    /** position in current data buffer + bytes flushed in transports */
    size_t m_DataAbsolutePosition = 0;

    /** position in metadata buffer */
    size_t m_MetadataPosition = 0;

    /**
     * Unique constructor
     * @param type derived class
     * @param debugMode true: extra exception checks
     */
    Capsule(const std::string type, const bool debugMode);

    virtual ~Capsule() = default;

    /** pointer to the raw data buffer */
    virtual char *GetData() = 0;
    /** pointer to the raw metadata buffer */
    virtual char *GetMetadata() = 0;

    virtual size_t GetDataSize() const = 0;     ///< data buffer memory size
    virtual size_t GetMetadataSize() const = 0; ///< metadata buffer memory size

    size_t GetAvailableDataSize() const;

    virtual void ResizeData(size_t size);     ///< resize data buffer
    virtual void ResizeMetadata(size_t size); ///< resize metadata buffer

protected:
    const bool m_DebugMode = false; ///< true: extra exception checks
};

} // end namespace

#endif /* ADIOS2_TOOLKIT_CAPSULE_CAPSULE_H_ */
