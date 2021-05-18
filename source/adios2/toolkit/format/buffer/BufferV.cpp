/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BufferV.cpp
 *
 */

#include "BufferV.h"
#include <string.h>

namespace adios2
{
namespace format
{

BufferV::BufferV(const std::string type) : m_Type(type) {}

size_t BufferV::AddToVec(const size_t size, const void *buf, int align,
                         bool CopyReqd)
{
    int badAlign = CurOffset % align;
    if (badAlign)
    {
        int addAlign = align - badAlign;
        char zero[16] = {0};
        AddToVec(addAlign, zero, 1, true);
    }
    size_t retOffset = CurOffset;

    if (size == 0)
        return CurOffset;

    if (!CopyReqd)
    {
        // just add buf to internal version of output vector
        VecEntry entry = {true, buf, 0, size};
        DataV.push_back(entry);
    }
    else
    {
        InternalBlock.Resize(m_internalPos + size, "");
        memcpy(InternalBlock.Data() + m_internalPos, buf, size);
        if (DataV.size() && !DataV.back().External &&
            (m_internalPos == (DataV.back().Offset + DataV.back().Size)))
        {
            // just add to the size of the existing tail entry
            DataV.back().Size += size;
        }
        else
        {
            DataV.push_back({false, NULL, m_internalPos, size});
        }
        m_internalPos += size;
    }
    CurOffset = retOffset + size;
    return retOffset;
}

uint64_t BufferV::Size() noexcept { return CurOffset; }

BufferV::BufferV_iovec BufferV::DataVec() noexcept
{
    BufferV_iovec ret = new iovec[DataV.size() + 1];
    for (std::size_t i = 0; i < DataV.size(); ++i)
    {
        if (DataV[i].External)
        {
            ret[i].iov_base = DataV[i].Base;
        }
        else
        {
            ret[i].iov_base = InternalBlock.Data() + DataV[i].Offset;
        }
        ret[i].iov_len = DataV[i].Size;
    }
    ret[DataV.size()] = {NULL, 0};
    return ret;
}
} // end namespace format
} // end namespace adios2
