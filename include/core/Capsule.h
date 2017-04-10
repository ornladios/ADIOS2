/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Capsule.h
 *
 *  Created on: Dec 7, 2016
 *      Author: wfgtemplates and pointers
 */

#ifndef CAPSULE_H_
#define CAPSULE_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <string>
/// \endcond

namespace adios
{

/**
 * Base class that raw data and metadata buffers, used by Engine.
 * Derived classes will allocate their own buffer in different memory spaces.
 * e.g. locally (heap) or in shared memory (virtual memory)
 */
class Capsule
{

public:
    const std::string m_Type;       ///< buffer type
    const std::string m_AccessMode; ///< 'w': write, 'r': read, 'a': append

    std::size_t m_DataPosition = 0; ///< position in current data buffer (not
                                    /// included data flushed to transports)
    std::size_t m_DataAbsolutePosition = 0; ///< include bytes flushed

    std::size_t m_MetadataPosition = 0; ///< position in metadata buffer

    /**
     * Base class constructor providing type from derived class and accessMode
     * @param type derived class type
     * @param accessMode 'w':write, 'r':read, 'a':append
     * @param rankMPI current MPI rank
     * @param debugMode
     */
    Capsule(const std::string type, const std::string accessMode,
            const int rankMPI, const bool debugMode);

    virtual ~Capsule() = default;

    virtual char *GetData() = 0; ///< return the pointer to the raw data buffer
    virtual char *
    GetMetadata() = 0; ///< return the pointer to the raw metadata buffer

    virtual std::size_t GetDataSize() const = 0;     ///< data buffer size
    virtual std::size_t GetMetadataSize() const = 0; ///< metadata buffer size

    virtual void ResizeData(std::size_t size);     ///< resize data buffer
    virtual void ResizeMetadata(std::size_t size); ///< resize metadata buffer

protected:
    const int m_RankMPI = 0;        ///< current MPI rank
    const bool m_DebugMode = false; ///< true: extra checks
};

} // end namespace

#endif /* CAPSULE_H_ */
