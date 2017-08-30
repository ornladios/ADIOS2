/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP1Reader.h
 *
 *  Created on: Sep 7, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP1_BP1READER_H_
#define ADIOS2_TOOLKIT_FORMAT_BP1_BP1READER_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <vector>
/// \endcond

#include "adios2/ADIOSConfig.h"
#include "adios2/ADIOSTypes.h"
#include "adios2/toolkit/format/bp1/BP1Base.h"

namespace adios2
{
namespace format
{

class BP1Reader : BP1Base
{

public:
    /**
     * Unique constructor
     * @param mpiComm
     * @param debug true: extra checks
     */
    BP1Reader(MPI_Comm mpiComm, const bool debugMode);

    ~BP1Reader() = default;
};

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_FORMAT_BP1_BP1READER_H_ */
