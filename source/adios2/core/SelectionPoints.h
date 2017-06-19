/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SelectionPoints.h
 *
 *  Created on: May 17, 2017
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 *              William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_CORE_SELECTIONPOINTS_H_
#define ADIOS2_CORE_SELECTIONPOINTS_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <vector>
/// \endcond

#include "adios2/ADIOSConfig.h"
#include "adios2/ADIOSTypes.h"
#include "adios2/core/Selection.h"

namespace adios2
{

/** Selection for a selection of an enumeration of positions.
 *  @param     ndim      Number of dimensions
 *  @param     npoints   Number of points of the selection
 *  @param     points    1D array of indices, compacted for all dimension
 *              (e.g.  [i1,j1,k1,i2,j2,k2,...,in,jn,kn] for
 *              n points in a 3D space.
 */
class SelectionPoints : public Selection
{
public:
    /** number of dimensions from constructor */
    const size_t m_DimensionsSize;

    /** reference to points in constructor, must use a pointer as nullptr is
     * valid if an array is passed in the constructor instead */
    std::vector<uint64_t> *m_PointsVec = nullptr;

    /** pointer based applications needed size*/
    const size_t m_PointsSize;
    /** pointer based applications needed array*/
    uint64_t *m_PointsPointer = nullptr;

    /**
     * Constructor that takes a vector<uint64_t> for points
     * @param dimensionsSize
     * @param pointsSize
     * @param points
     */
    SelectionPoints(size_t dimensionsSize, std::vector<uint64_t> &points);

    SelectionPoints(size_t dimensionsSize, size_t pointsSize, uint64_t *points);

    virtual ~SelectionPoints() = default;
};

} // end namespace adios

#endif /* ADIOS2_CORE_SELECTIONPOINTS_H_ */
