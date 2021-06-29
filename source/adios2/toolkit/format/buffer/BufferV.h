/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BUFFER_BUFFERV_H_
#define ADIOS2_TOOLKIT_FORMAT_BUFFER_BUFFERV_H_

#include "adios2/common/ADIOSConfig.h"
#include "adios2/common/ADIOSTypes.h"

namespace adios2
{
namespace format
{

class BufferV
{
public:
    const std::string m_Type;

    typedef struct iovec
    {
        const void
            *iov_base;  //  Base address of a memory region for input or output.
        size_t iov_len; //  The size of the memory pointed to by iov_base.
    } * BufferV_iovec;

    uint64_t Size() noexcept;

    BufferV(const std::string type, const bool AlwaysCopy = false);
    virtual ~BufferV();

    virtual BufferV_iovec DataVec() noexcept = 0;

    /**
     * Reset the buffer to initial state (without freeing internal buffers)
     */
    virtual void Reset();

    virtual size_t AddToVec(const size_t size, const void *buf, int align,
                            bool CopyReqd) = 0;

public:
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
