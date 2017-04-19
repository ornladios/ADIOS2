/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BZip2.h
 *
 *  Created on: Oct 17, 2016
 *      Author: wfg
 */

#ifndef BZIP2_H_
#define BZIP2_H_

#include "adios2/ADIOSConfig.h"
#include "adios2/core/Transform.h"

namespace adios
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

#endif /* BZIP2_H_ */
