/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Selection.cpp : Public C++ bindings for Selection class
 */

#include "Selection.h"
#include "adios2/core/Selection.h"

namespace adios2
{

class Selection::Impl
{
public:
    core::Selection m_Selection;
};

Selection::Selection() : m_Impl(std::make_shared<Impl>()) {}

//============================================================================
// Factory methods
//============================================================================

Selection Selection::All()
{
    Selection sel;
    sel.m_Impl->m_Selection = core::Selection::All();
    return sel;
}

Selection Selection::BoundingBox(const Dims &start, const Dims &count)
{
    Selection sel;
    sel.m_Impl->m_Selection = core::Selection::BoundingBox(start, count);
    return sel;
}

Selection Selection::BoundingBox(const Box<Dims> &box)
{
    return BoundingBox(box.first, box.second);
}

Selection Selection::Block(size_t blockID)
{
    Selection sel;
    sel.m_Impl->m_Selection = core::Selection::Block(blockID);
    return sel;
}

//============================================================================
// Mutating setters
//============================================================================

Selection &Selection::SetBoundingBox(const Dims &start, const Dims &count)
{
    m_Impl->m_Selection.SetBoundingBox(start, count);
    return *this;
}

Selection &Selection::SetBoundingBox(const Box<Dims> &box)
{
    return SetBoundingBox(box.first, box.second);
}

Selection &Selection::SetBlock(size_t blockID)
{
    m_Impl->m_Selection.SetBlock(blockID);
    return *this;
}

Selection &Selection::ClearBlock()
{
    m_Impl->m_Selection.ClearBlock();
    return *this;
}

Selection &Selection::SetSteps(size_t stepStart, size_t stepCount)
{
    m_Impl->m_Selection.SetSteps(stepStart, stepCount);
    return *this;
}

Selection &Selection::SetSteps(const Box<size_t> &steps)
{
    return SetSteps(steps.first, steps.second);
}

Selection &Selection::SetMemory(const Dims &memoryStart, const Dims &memoryCount)
{
    m_Impl->m_Selection.SetMemory(memoryStart, memoryCount);
    return *this;
}

Selection &Selection::SetMemory(const Box<Dims> &memory)
{
    return SetMemory(memory.first, memory.second);
}

Selection &Selection::ClearMemory()
{
    m_Impl->m_Selection.ClearMemory();
    return *this;
}

Selection &Selection::SetAccuracy(const Accuracy &accuracy)
{
    m_Impl->m_Selection.SetAccuracy(accuracy);
    return *this;
}

Selection &Selection::SetAccuracy(double error, double norm, bool relative)
{
    return SetAccuracy(Accuracy{error, norm, relative});
}

void Selection::Clear() { m_Impl->m_Selection.Clear(); }

//============================================================================
// Non-mutating modifiers
//============================================================================

Selection Selection::WithBlock(size_t blockID) const
{
    Selection sel;
    sel.m_Impl->m_Selection = m_Impl->m_Selection.WithBlock(blockID);
    return sel;
}

Selection Selection::WithSteps(size_t stepStart, size_t stepCount) const
{
    Selection sel;
    sel.m_Impl->m_Selection = m_Impl->m_Selection.WithSteps(stepStart, stepCount);
    return sel;
}

Selection Selection::WithSteps(const Box<size_t> &steps) const
{
    return WithSteps(steps.first, steps.second);
}

Selection Selection::WithMemory(const Dims &memoryStart, const Dims &memoryCount) const
{
    Selection sel;
    sel.m_Impl->m_Selection = m_Impl->m_Selection.WithMemory(memoryStart, memoryCount);
    return sel;
}

Selection Selection::WithMemory(const Box<Dims> &memory) const
{
    return WithMemory(memory.first, memory.second);
}

Selection Selection::WithAccuracy(const Accuracy &accuracy) const
{
    Selection sel;
    sel.m_Impl->m_Selection = m_Impl->m_Selection.WithAccuracy(accuracy);
    return sel;
}

Selection Selection::WithAccuracy(double error, double norm, bool relative) const
{
    return WithAccuracy(Accuracy{error, norm, relative});
}

std::string Selection::ToString() const { return m_Impl->m_Selection.ToString(); }

const core::Selection &Selection::GetCoreSelection() const { return m_Impl->m_Selection; }

} // end namespace adios2
