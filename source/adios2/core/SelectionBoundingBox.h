/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SelectionBoundingBox.h
 *
 *  Created on: May 17, 2017
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 *              William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_CORE_SELECTIONBOUNDINGBOX_H_
#define ADIOS2_CORE_SELECTIONBOUNDINGBOX_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <vector>
/// \endcond

#include "adios2/ADIOSConfig.h"
#include "adios2/core/Selection.h"

namespace adios
{

/** Bounding box selection to read a subset of a non-scalar variable.
 *  @param start     array of offsets to start reading in each dimension
 *  @param count     number of data elements to read in each dimension
 */
class SelectionBoundingBox : public Selection
{
public:
    /** Starting coodinates of bounding box */
    Dims m_Start;
    /** From start of bounding box */
    Dims m_Count;

    /**
     * Constructor using uint64_t vectors
     * @param start of bounding box
     * @param count from start of bounding box
     */
    //    SelectionBoundingBox(const std::vector<uint64_t> start,
    //                         const std::vector<uint64_t> count,
    //                         const bool debugMode = false);

    /**
     * Constructor using size_t vectors
     * @param start of bounding box
     * @param count from start of bounding box
     */
    SelectionBoundingBox(const Dims start, const Dims count,
                         const bool debugMode = false);

    ~SelectionBoundingBox() = default;

private:
    /** Check in debug mode to make sure bounding box is valid */
    void CheckBoundingBox();
};

} // end namespace adios

#endif /* ADIOS2_CORE_SELECTIONBOUNDINGBOX_H_ */
