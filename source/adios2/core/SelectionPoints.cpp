/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SelectionPoints.cpp
 *
 *  Created on: May 17, 2017
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 *              William F Godoy godoywf@ornl.gov
 */

#include "SelectionPoints.h"

namespace adios
{

SelectionPoints::SelectionPoints(size_t dimensionsSize,
                                 std::vector<uint64_t> &points)
: Selection(SelectionType::Points), m_DimensionsSize(dimensionsSize),
  m_PointsVec(&points), m_PointsSize(points.size())
{
}

SelectionPoints::SelectionPoints(size_t dimensionsSize, size_t pointsSize,
                                 uint64_t *points)
: Selection(SelectionType::Points), m_DimensionsSize(dimensionsSize),
  m_PointsSize(pointsSize), m_PointsPtr(points)
{
}

} // end namespace adios
