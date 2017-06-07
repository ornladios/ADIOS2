/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SelectionBoundingBox.cpp
 *
 *  Created on: May 17, 2017
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 *              William F Godoy godoywf@ornl.gov
 *
 */

#include "SelectionBoundingBox.h"

/// \cond EXCLUDE_FROM_DOXYGEN
#include <stdexcept>
/// \endcond

#include "adios2/helper/adiosFunctions.h" //Uint64VectorToSizetVector

namespace adios
{

SelectionBoundingBox::SelectionBoundingBox(const Dims start, const Dims count,
                                           const bool debugMode)
: Selection(SelectionType::BoundingBox, debugMode), m_Start(start),
  m_Count(count)
{
    if (m_DebugMode)
    {
        CheckBoundingBox();
    }
}

void SelectionBoundingBox::CheckBoundingBox() const
{

    auto lf_Throw = [](const std::string &message) {
        throw std::invalid_argument(
            "ERROR: " + message +
            ", in call to SelectionBoundingBox constructor\n");
    };

    if (m_Start.size() != m_Count.size())
    {
        lf_Throw("start and count must have the same size");
    }

    if (m_Start.empty())
    {
        lf_Throw("start is empty");
    }

    if (m_Count.empty())
    {
        lf_Throw("count is empty");
    }
}

} // end namespace adios
