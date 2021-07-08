/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BUFFER_MALLOC_MALLOCV_H_
#define ADIOS2_TOOLKIT_FORMAT_BUFFER_MALLOC_MALLOCV_H_

#include "adios2/common/ADIOSConfig.h"
#include "adios2/common/ADIOSTypes.h"

#include "adios2/toolkit/format/buffer/BufferV.h"

namespace adios2
{
namespace format
{

class MallocV : public BufferV
{
public:
    uint64_t Size() noexcept;

    MallocV(const std::string type, const bool AlwaysCopy = false,
            size_t InitialBufferSize = DefaultInitialBufferSize,
            double GrowthFactor = DefaultBufferGrowthFactor);
    virtual ~MallocV();

    virtual BufferV_iovec DataVec() noexcept;

    /**
     * Reset the buffer to initial state (without freeing internal buffers)
     */
    virtual void Reset();

    virtual size_t AddToVec(const size_t size, const void *buf, int align,
                            bool CopyReqd);

    void CopyExternalToInternal();

private:
    char *m_InternalBlock = NULL;
    size_t m_AllocatedSize = 0;
    const size_t m_InitialBufferSize = 16 * 1024;
    const double m_GrowthFactor = 1.05;
};

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_FORMAT_BUFFER_MALLOC_MALLOCV_H_ */
