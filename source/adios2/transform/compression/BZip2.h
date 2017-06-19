/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BZip2.h
 *
 *  Created on: Oct 17, 2016
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TRANSFORM_COMPRESSION_BZIP2_H_
#define ADIOS2_TRANSFORM_COMPRESSION_BZIP2_H_

#include "adios2/ADIOSConfig.h"
#include "adios2/core/Transform.h"

namespace adios2
{
namespace transform
{

class BZip2 : public Transform
{

public:
    /**
     * Initialize parent method
     * @param compressionLevel
     * @param variable
     */
    BZip2();

    virtual ~BZip2() = default;

    void Compress(const std::vector<char> &bufferIn,
                  std::vector<char> &bufferOut);

    void Decompress(const std::vector<char> &bufferIn,
                    std::vector<char> &bufferOut);
};

} // end namespace transform
} // end namespace adios

#endif /* ADIOS2_TRANSFORM_BZIP2_H_ */
