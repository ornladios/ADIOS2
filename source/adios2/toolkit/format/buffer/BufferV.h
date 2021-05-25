/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BUFFER_BUFFERV_H_
#define ADIOS2_TOOLKIT_FORMAT_BUFFER_BUFFERV_H_

#include "adios2/common/ADIOSConfig.h"
#include "adios2/common/ADIOSTypes.h"
#include "heap/BufferSTL.h"

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

    BufferV(const std::string type);
    virtual ~BufferV() = default;

    virtual BufferV_iovec DataVec() noexcept;
    //  virtual const BufferV_iovec DataVec() const noexcept;

    virtual size_t AddToVec(const size_t size, const void *buf, int align,
                            bool CopyReqd);

private:
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
    BufferSTL InternalBlock;
};

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_FORMAT_BUFFER_BUFFERV_H_ */
