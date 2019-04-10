
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

std::string ToString(ShapeID value)
{
    switch (value)
    {
    case ShapeID::Unknown:
        return "ShapeID::Unknown";
    case ShapeID::GlobalValue:
        return "ShapeID::GlobalValue";
    case ShapeID::GlobalArray:
        return "ShapeID::GlobalArray";
    case ShapeID::JoinedArray:
        return "ShapeID::JoinedArray";
    case ShapeID::LocalValue:
        return "ShapeID::LocalValue";
    case ShapeID::LocalArray:
        return "ShapeID::LocalArray";
    default:
        return "ToString: Unknown ShapeID";
    }
}

std::string ToString(IOMode value)
{
    switch (value)
    {
    case IOMode::Independent:
        return "IOMode::Independent";
    case IOMode::Collective:
        return "IOMode::Collective";
    default:
        return "ToString: Unknown IOMode";
    }
}

std::string ToString(Mode value)
{
    switch (value)
    {
    case Mode::Undefined:
        return "Mode::Undefined";
    case Mode::Write:
        return "Mode::Write";
    case Mode::Read:
        return "Mode::Read";
    case Mode::Append:
        return "Mode::Append";
    case Mode::Sync:
        return "Mode::Sync";
    case Mode::Deferred:
        return "Mode::Deferred";
    default:
        return "ToString: Unknown Mode";
    }
}

} // end namespace adios2
