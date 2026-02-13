/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Selection.cpp : Implementation of Selection class
 */

#include "Selection.h"
#include "adios2/helper/adiosFunctions.h"

#include <sstream>

namespace adios2
{
namespace core
{

//============================================================================
// Factory methods
//============================================================================

Selection Selection::All()
{
    Selection sel;
    // Default constructor already sets m_Type = SelectionType::All
    return sel;
}

Selection Selection::BoundingBox(const Dims &start, const Dims &count)
{
    Selection sel;
    sel.SetBoundingBox(start, count);
    return sel;
}

Selection Selection::BoundingBox(const Box<Dims> &box)
{
    return BoundingBox(box.first, box.second);
}

Selection Selection::Block(size_t blockID)
{
    Selection sel;
    sel.SetBlock(blockID);
    return sel;
}

//============================================================================
// Mutating setters
//============================================================================

Selection &Selection::SetBoundingBox(const Dims &start, const Dims &count)
{
    if (start.size() != count.size())
    {
        helper::Throw<std::invalid_argument>("Core", "Selection", "SetBoundingBox",
                                             "start and count must have the same dimensions");
    }

    m_Type = SelectionType::BoundingBox;
    m_Start = start;
    m_Count = count;
    return *this;
}

Selection &Selection::SetBoundingBox(const Box<Dims> &box)
{
    return SetBoundingBox(box.first, box.second);
}

Selection &Selection::SetBlock(size_t blockID)
{
    m_Type = SelectionType::WriteBlock;
    m_BlockID = blockID;
    m_Start.clear();
    m_Count.clear();
    return *this;
}

Selection &Selection::SetSteps(size_t stepStart, size_t stepCount)
{
    m_StepStart = stepStart;
    m_StepCount = stepCount;
    return *this;
}

Selection &Selection::SetSteps(const Box<size_t> &steps)
{
    return SetSteps(steps.first, steps.second);
}

Selection &Selection::SetMemory(const Dims &memoryStart, const Dims &memoryCount)
{
    if (memoryStart.size() != memoryCount.size())
    {
        helper::Throw<std::invalid_argument>("Core", "Selection", "SetMemory",
                                             "memoryStart and memoryCount must have the same "
                                             "dimensions");
    }

    m_MemoryStart = memoryStart;
    m_MemoryCount = memoryCount;
    m_HasMemory = true;
    return *this;
}

Selection &Selection::SetMemory(const Box<Dims> &memory)
{
    return SetMemory(memory.first, memory.second);
}

Selection &Selection::ClearMemory()
{
    m_HasMemory = false;
    m_MemoryStart.clear();
    m_MemoryCount.clear();
    return *this;
}

Selection &Selection::SetAccuracy(const Accuracy &accuracy)
{
    m_Accuracy = accuracy;
    return *this;
}

Selection &Selection::SetAccuracy(double error, double norm, bool relative)
{
    m_Accuracy = Accuracy{error, norm, relative};
    return *this;
}

void Selection::Clear()
{
    m_Type = SelectionType::All;
    m_Start.clear();
    m_Count.clear();
    m_BlockID = 0;
    m_StepStart = 0;
    m_StepCount = 1;
    m_MemoryStart.clear();
    m_MemoryCount.clear();
    m_HasMemory = false;
    m_Accuracy = {0.0, 0.0, false};
}

//============================================================================
// Non-mutating modifiers (return copies)
//============================================================================

Selection Selection::WithSteps(size_t stepStart, size_t stepCount) const
{
    Selection sel = *this;
    sel.SetSteps(stepStart, stepCount);
    return sel;
}

Selection Selection::WithSteps(const Box<size_t> &steps) const
{
    return WithSteps(steps.first, steps.second);
}

Selection Selection::WithMemory(const Dims &memoryStart, const Dims &memoryCount) const
{
    Selection sel = *this;
    sel.SetMemory(memoryStart, memoryCount);
    return sel;
}

Selection Selection::WithMemory(const Box<Dims> &memory) const
{
    return WithMemory(memory.first, memory.second);
}

Selection Selection::WithAccuracy(const Accuracy &accuracy) const
{
    Selection sel = *this;
    sel.SetAccuracy(accuracy);
    return sel;
}

Selection Selection::WithAccuracy(double error, double norm, bool relative) const
{
    return WithAccuracy(Accuracy{error, norm, relative});
}

std::string Selection::ToString() const
{
    std::ostringstream os;
    os << "Selection(";

    switch (m_Type)
    {
    case SelectionType::All:
        os << "All";
        break;
    case SelectionType::BoundingBox:
        os << "BoundingBox start={";
        for (size_t i = 0; i < m_Start.size(); ++i)
        {
            if (i > 0)
            {
                os << ", ";
            }
            os << m_Start[i];
        }
        os << "} count={";
        for (size_t i = 0; i < m_Count.size(); ++i)
        {
            if (i > 0)
            {
                os << ", ";
            }
            os << m_Count[i];
        }
        os << "}";
        break;
    case SelectionType::WriteBlock:
        os << "Block " << m_BlockID;
        break;
    }

    if (m_StepStart != 0 || m_StepCount != 1)
    {
        os << ", steps=[" << m_StepStart << ", " << m_StepCount << "]";
    }
    if (m_HasMemory)
    {
        os << ", memory start={";
        for (size_t i = 0; i < m_MemoryStart.size(); ++i)
        {
            if (i > 0)
            {
                os << ", ";
            }
            os << m_MemoryStart[i];
        }
        os << "} count={";
        for (size_t i = 0; i < m_MemoryCount.size(); ++i)
        {
            if (i > 0)
            {
                os << ", ";
            }
            os << m_MemoryCount[i];
        }
        os << "}";
    }
    if (m_Accuracy.error != 0.0 || m_Accuracy.norm != 0.0)
    {
        os << ", accuracy={" << m_Accuracy.error << ", " << m_Accuracy.norm << ", "
           << (m_Accuracy.relative ? "relative" : "absolute") << "}";
    }

    os << ")";
    return os.str();
}

} // end namespace core
} // end namespace adios2
