/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BufferSTL.h
 *
 *  Created on: Sep 26, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BUFFERSTL_H_
#define ADIOS2_TOOLKIT_FORMAT_BUFFERSTL_H_

#include <string>
#include <vector>

#include "adios2/common/ADIOSTypes.h"

namespace adios2
{

class BufferSTL
{
public:
    std::vector<char> m_Buffer;
    size_t m_Position = 0;
    size_t m_AbsolutePosition = 0;

    BufferSTL() = default;
    ~BufferSTL() = default;

    void Resize(const size_t size, const std::string hint);

    size_t GetAvailableSize() const;

private:
    const bool m_DebugMode = false;
};

} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_FORMAT_STLBUFFER_H_ */
