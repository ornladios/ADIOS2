/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#ifndef ADIOS2_TOOLKIT_CAPSULE_SHMEM_SHMSYSTEMV_H_
#define ADIOS2_TOOLKIT_CAPSULE_SHMEM_SHMSYSTEMV_H_

#include <sys/ipc.h>
#include <sys/types.h>

#include "adios2/ADIOSConfig.h"
#include "adios2/toolkit/capsule/Capsule.h"

namespace adios
{

/**
 * Buffer and Metadata are allocated in virtual memory using interprocess
 * communication (IPC) of Unix's System V
 */
class ShmSystemV : public Capsule
{

public:
    /**
         * Create a Capsule in shared memory using System V shm API
         * @param accessMode
         * @param pathName used to create the key as a unique identifier
         * @param dataSize size of allocated memory segment for data
         * @param metadataSize size of allocated memory segment for metadata
         * @param debugMode true: extra checks, slower
         */
    ShmSystemV(const std::string &pathName, const int rankMPI,
               const size_t dataSize, const size_t metadataSize,
               const bool debugMode = false);

    ~ShmSystemV() = default;

    char *GetData();     ///< return the pointer to the raw data buffer
    char *GetMetadata(); ///< return the pointer to the raw metadata buffer

    size_t GetDataSize() const;     ///< get current data buffer size
    size_t GetMetadataSize() const; ///< get current metadata buffer size

private:
    /** reference to a shared memory data buffer */
    char *m_Data = nullptr;
    const size_t m_DataSize; ///< size of the allocated shared memory segment
    key_t m_DataKey; ///< key associated with the data buffer, created with ftok
    int m_DataShmID; ///< data shared memory buffer id

    /** reference to a shared memory metadata buffer created with shmget */
    char *m_Metadata = nullptr;
    const size_t m_MetadataSize; ///< size of the allocated segment
    key_t m_MetadataKey;         ///< ftok metadata buffer key
    int m_MetadataShmID;         ///< metadata shared memory buffer id

    /** checks if all shared memory allocations are correct, throws
       std::bad_alloc, called from constructor if debug mode is true */
    void CheckShm() const;
};

} // end namespace adios

#endif /* ADIOS2_TOOLKIT_CAPSULE_SHMEM_SHMSYSTEMV_H_ */
