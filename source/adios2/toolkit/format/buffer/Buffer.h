/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Buffer.h : abstract class for all buffer memory types
 *
 *  Created on: Jul 9, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BUFFER_BUFFER_H_
#define ADIOS2_TOOLKIT_FORMAT_BUFFER_BUFFER_H_

#include "adios2/common/ADIOSConfig.h"
#include "adios2/common/ADIOSTypes.h"

namespace adios2
{
namespace format
{

class Buffer
{
public:
    const std::string m_Type;
    size_t m_Position = 0;
    size_t m_AbsolutePosition = 0;

    /** if 0: buffer can be extended, if >0: buffer has a fixed size */
    size_t m_FixedSize = 0;

    Buffer(const std::string type, const bool debugMode);
    virtual ~Buffer() = default;

    virtual void Resize(const size_t size, const std::string hint);

    virtual size_t GetAvailableSize() const = 0;

private:
    const bool m_DebugMode;
};

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_FORMAT_BUFFER_BUFFER_H_ */
