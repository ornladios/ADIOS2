
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

std::string ToString(ReadMultiplexPattern value)
{
    switch (value)
    {
    case ReadMultiplexPattern::GlobalReaders:
        return "ReadMultiplexPattern::GlobalReaders";
    case ReadMultiplexPattern::RoundRobin:
        return "ReadMultiplexPattern::RoundRobin";
    case ReadMultiplexPattern::FirstInFirstOut:
        return "ReadMultiplexPattern::FirstInFirstOut";
    case ReadMultiplexPattern::OpenAllSteps:
        return "ReadMultiplexPattern::OpenAllSteps";
    default:
        return "ToString: Unknown ReadMultiplexPattern";
    }
}

std::string ToString(StreamOpenMode value)
{
    switch (value)
    {
    case StreamOpenMode::Wait:
        return "StreamOpenMode::Wait";
    case StreamOpenMode::NoWait:
        return "StreamOpenMode::NoWait";
    default:
        return "ToString: Unknown StreamOpenMode";
    }
}

std::string ToString(ReadMode value)
{
    switch (value)
    {
    case ReadMode::NonBlocking:
        return "ReadMode::NonBlocking";
    case ReadMode::Blocking:
        return "ReadMode::Blocking";
    default:
        return "ToString: Unknown ReadMode";
    }
}

std::string ToString(StepMode value)
{
    switch (value)
    {
    case StepMode::Append:
        return "StepMode::Append";
    case StepMode::Update:
        return "StepMode::Update";
    case StepMode::Read:
        return "StepMode::Read";
    default:
        return "ToString: Unknown StepMode";
    }
}

std::string ToString(StepStatus value)
{
    switch (value)
    {
    case StepStatus::OK:
        return "StepStatus::OK";
    case StepStatus::NotReady:
        return "StepStatus::NotReady";
    case StepStatus::EndOfStream:
        return "StepStatus::EndOfStream";
    case StepStatus::OtherError:
        return "StepStatus::OtherError";
    default:
        return "ToString: Unknown StepStatus";
    }
}

std::string ToString(TimeUnit value)
{
    switch (value)
    {
    case TimeUnit::Microseconds:
        return "TimeUnit::Microseconds";
    case TimeUnit::Milliseconds:
        return "TimeUnit::Milliseconds";
    case TimeUnit::Seconds:
        return "TimeUnit::Seconds";
    case TimeUnit::Minutes:
        return "TimeUnit::Minutes";
    case TimeUnit::Hours:
        return "TimeUnit::Hours";
    default:
        return "ToString: Unknown TimeUnit";
    }
}

std::string ToString(SelectionType value)
{
    switch (value)
    {
    case SelectionType::BoundingBox:
        return "SelectionType::BoundingBox";
    case SelectionType::Points:
        return "SelectionType::Points";
    case SelectionType::WriteBlock:
        return "SelectionType::WriteBlock";
    case SelectionType::Auto:
        return "SelectionType::Auto";
    default:
        return "ToString: Unknown SelectionType";
    }
}

} // end namespace adios2
