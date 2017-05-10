/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Selection.cpp
 *
 *  Created on: May 19, 2017
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 *              William F Godoy godoywf@ornl.gov
 */

#include "Selection.h"

namespace adios
{

Selection::Selection(const SelectionType type, const bool debugMode)
: m_Type(type), m_DebugMode(debugMode)
{
}

} // end namespace adios
