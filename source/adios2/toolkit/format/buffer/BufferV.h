/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BUFFER_BUFFERV_H_
#define ADIOS2_TOOLKIT_FORMAT_BUFFER_BUFFERV_H_

#include "adios2/common/ADIOSConfig.h"
#include "adios2/common/ADIOSTypes.h"
#include "adios2/core/CoreTypes.h"
#include <iostream>

namespace adios2
{
namespace format
{

class BufferV
{
public:
    const std::string m_Type;

    uint64_t Size() noexcept;

    BufferV(const std::string type, const bool AlwaysCopy = false);
    virtual ~BufferV();

    virtual std::vector<core::iovec> DataVec() noexcept = 0;

    /*
     *  This is used in PerformPuts() to copy externally referenced data so that
     * it can be modified by the application
     */
    virtual void CopyExternalToInternal() = 0;

    /**
     * Reset the buffer to initial state (without freeing internal buffers)
     */
    virtual void Reset();

    virtual size_t AddToVec(const size_t size, const void *buf, size_t align,
                            bool CopyReqd) = 0;

    struct BufferPos
    {
        int bufferIdx = -1;     // buffer index
        size_t posInBuffer = 0; // position in buffer[idx]
        size_t globalPos = 0;   // global position in virtual buffer
        BufferPos(int idx, size_t pos, size_t globalPos)
        : bufferIdx(idx), posInBuffer(pos), globalPos(globalPos){};
    };

    /** Allocate size bytes and return BufferPos position.
     * Used by Span functions to allocate memory on behalf of the user
     * Return both the position in the virtual memory buffer as well
     * as all info needed to retrieve a valid pointer any time
     * during execution (even after reallocs)
     */
    virtual BufferPos Allocate(const size_t size, size_t align) = 0;

    void AlignBuffer(const size_t align);

    virtual void *GetPtr(int bufferIdx, size_t posInBuffer) = 0;

protected:
    static char zero[64];
    const bool m_AlwaysCopy = false;

    struct VecEntry
    {
        bool External;
        const void *Base;
        size_t Offset;
        size_t Size;
    };
    std::vector<VecEntry> DataV;
    size_t CurOffset = 0;
    size_t m_internalPos = 0;
};

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_FORMAT_BUFFER_BUFFERV_H_ */
