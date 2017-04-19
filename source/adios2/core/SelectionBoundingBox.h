/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#ifndef __ADIOS_SELECTION_BOUNDINGBOX_H__
#define __ADIOS_SELECTION_BOUNDINGBOX_H__

/// \cond EXCLUDE_FROM_DOXYGEN
#include <cstdint>
/// \endcond

#include <vector>

#include "adios2/ADIOSConfig.h"
#include "adios2/core/Selection.h"

namespace adios
{

/** Boundingbox selection to read a subset of a non-scalar variable.
 *  @param start     array of offsets to start reading in each dimension
 *  @param count     number of data elements to read in each dimension
 */
class SelectionBoundingBox : public Selection
{
public:
    SelectionBoundingBox(const std::vector<std::uint64_t> start,
                         const std::vector<std::uint64_t> count)
    : Selection(SelectionType::BoundingBox), m_Start(start), m_Count(count)
    {
    }

    ~SelectionBoundingBox() = default;

    std::vector<std::uint64_t> m_Start;
    std::vector<std::uint64_t> m_Count;
};

} // namespace adios

#endif /*__ADIOS_SELECTION_BOUNDINGBOX_H__*/
