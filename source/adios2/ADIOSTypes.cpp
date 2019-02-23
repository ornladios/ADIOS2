
/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOSTypes.cpp: implementation of enum-related functions
 *
 *  Created on: Feb 22, 2019
 *      Author: Kai Germaschewski <kai.germaschewski@unh.edu>
 */

#include "ADIOSTypes.h"

namespace adios2
{

namespace
{

std::map<ShapeID, std::string> MapShapeID2String = {
    {ShapeID::Unknown, "ShapeID::Unknown"},
    {ShapeID::GlobalValue, "ShapeID::GlobalValue"},
    {ShapeID::GlobalArray, "ShapeID::GlobalArray"},
    {ShapeID::JoinedArray, "ShapeID::JoinedArray"},
    {ShapeID::LocalValue, "ShapeID::LocalValue"},
    {ShapeID::LocalArray, "ShapeID::LocalArray"},
};

} // end anonymous namespace

std::string ToString(ShapeID value) { return MapShapeID2String.at(value); }

} // end namespace adios2
