/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Selection.h : Thread-safe selection specification for Get() operations
 */

#ifndef ADIOS2_CORE_SELECTION_H_
#define ADIOS2_CORE_SELECTION_H_

#include "adios2/common/ADIOSTypes.h"

#include <string>

namespace adios2
{
namespace core
{

class VariableBase;

/**
 * Selection specification for Get() operations.
 *
 * Spatial selection (All or BoundingBox) and block selection are orthogonal.
 * A block ID can be combined with either spatial type:
 *   - Block(3)                         = All + block 3
 *   - BoundingBox({0},{10}).WithBlock(3) = sub-region of block 3
 *
 * Supports two usage patterns:
 * 1. Immutable/fluent: Use factory methods + With*() to create new instances
 * 2. Mutable/reuse: Use Set*() methods to modify in place
 *
 * A default-constructed Selection has type All, meaning the entire variable.
 */
class Selection
{
public:
    Selection() = default;

    //========================================================================
    // Factory methods (create new Selection)
    //========================================================================

    /** Create a selection that reads the entire variable. */
    static Selection All();

    /**
     * Create a bounding box selection (hyperslab).
     * @param start  Starting offsets in each dimension
     * @param count  Number of elements in each dimension
     */
    static Selection BoundingBox(const Dims &start, const Dims &count);
    static Selection BoundingBox(const Box<Dims> &box);

    /**
     * Create a block selection to read an individual write block.
     * Equivalent to All().WithBlock(blockID).
     * @param blockID  Block index from the writing rank
     */
    static Selection Block(size_t blockID);

    //========================================================================
    // Mutating setters (modify in place, return *this for chaining)
    //========================================================================

    Selection &SetBoundingBox(const Dims &start, const Dims &count);
    Selection &SetBoundingBox(const Box<Dims> &box);

    /** Set block ID without changing spatial selection type. */
    Selection &SetBlock(size_t blockID);
    Selection &ClearBlock();

    Selection &SetSteps(size_t stepStart, size_t stepCount);
    Selection &SetSteps(const Box<size_t> &steps);

    Selection &SetMemory(const Dims &memoryStart, const Dims &memoryCount);
    Selection &SetMemory(const Box<Dims> &memory);
    Selection &ClearMemory();

    Selection &SetAccuracy(const Accuracy &accuracy);
    Selection &SetAccuracy(double error, double norm = L2_norm, bool relative = false);

    /** Reset to default state */
    void Clear();

    //========================================================================
    // Non-mutating modifiers (return NEW Selection, original unchanged)
    //========================================================================

    Selection WithBlock(size_t blockID) const;

    Selection WithSteps(size_t stepStart, size_t stepCount) const;
    Selection WithSteps(const Box<size_t> &steps) const;

    Selection WithMemory(const Dims &memoryStart, const Dims &memoryCount) const;
    Selection WithMemory(const Box<Dims> &memory) const;

    Selection WithAccuracy(const Accuracy &accuracy) const;
    Selection WithAccuracy(double error, double norm = L2_norm, bool relative = false) const;

    //========================================================================
    // Accessors
    //========================================================================

    /** Returns All or BoundingBox (spatial selection type). */
    SelectionType GetSelectionType() const noexcept { return m_Type; }

    const Dims &GetStart() const noexcept { return m_Start; }
    const Dims &GetCount() const noexcept { return m_Count; }

    bool HasBlockSelection() const noexcept { return m_HasBlock; }
    size_t GetBlockID() const noexcept { return m_BlockID; }

    size_t GetStepStart() const noexcept { return m_StepStart; }
    size_t GetStepCount() const noexcept { return m_StepCount; }

    bool HasMemorySelection() const noexcept { return m_HasMemory; }
    const Dims &GetMemoryStart() const noexcept { return m_MemoryStart; }
    const Dims &GetMemoryCount() const noexcept { return m_MemoryCount; }

    const Accuracy &GetAccuracy() const noexcept { return m_Accuracy; }

    /** Human-readable string representation */
    std::string ToString() const;

private:
    SelectionType m_Type = SelectionType::All;
    Dims m_Start;
    Dims m_Count;
    bool m_HasBlock = false;
    size_t m_BlockID = 0;
    size_t m_StepStart = 0;
    size_t m_StepCount = 1;
    Dims m_MemoryStart;
    Dims m_MemoryCount;
    bool m_HasMemory = false;
    Accuracy m_Accuracy = {0.0, 0.0, false};
};

/**
 * Build a Selection from the current state of a VariableBase.
 * Reads m_SelectionType, m_Start, m_Count, m_BlockID, m_StepsStart,
 * m_StepsCount, m_MemoryStart, m_MemoryCount, and GetAccuracyRequested().
 */
Selection InferSelection(const VariableBase &variable);

} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_CORE_SELECTION_H_ */
