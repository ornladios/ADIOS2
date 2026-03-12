/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BUFFER_HEAP_BUFFERFFS_H_
#define ADIOS2_TOOLKIT_FORMAT_BUFFER_HEAP_BUFFERFFS_H_

#include "adios2/toolkit/format/buffer/Buffer.h"

#include "adios2/common/ADIOSMacros.h"
#include "ffs.h"
#include "fm.h"

namespace adios2
{
namespace format
{

class BufferFFS : public Buffer
{
public:
    FFSBuffer m_buffer = NULL;
    void *m_data = NULL;
    BufferFFS(FFSBuffer Buf, void *data, size_t length);
    ~BufferFFS();

    char *Data() noexcept final;
    const char *Data() const noexcept final;

    void Delete();
};

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_FORMAT_BUFFER_HEAP_BUFFERFFS_H_ */
