/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CoreTypes.h : types used only in the core framework, in contrast to
 *               ADIOSTypes.h, which is a public user-facing header
 *
 *  Created on: Aug 11, 2021
 *      Author:  Norbert Podhorszki pnorbert@ornl.gov
 */

#ifndef ADIOS2_CORETYPES_H_
#define ADIOS2_CORETYPES_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <cstddef>
#include <cstdint>
/// \endcond

#include "adios2/common/ADIOSConfig.h"

namespace adios2
{
namespace core
{

struct iovec
{
    //  Base address of a memory region for input or output.
    const void *iov_base;
    //  The size of the memory pointed to by iov_base.
    size_t iov_len;
};

} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_CORETYPES_H_ */
