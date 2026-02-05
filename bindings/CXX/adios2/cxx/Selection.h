/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Selection.h : Public C++ bindings for Selection class
 */

#ifndef ADIOS2_BINDINGS_CXX_CXX_SELECTION_H_
#define ADIOS2_BINDINGS_CXX_CXX_SELECTION_H_

#include "adios2/common/ADIOSTypes.h"

namespace adios2
{

namespace core
{
class Selection;
} // namespace core

/**
 * Selection specification for Get() operations.
 *
 * Supports two usage patterns:
 * 1. Immutable/fluent: Use factory methods + With*() to create new instances
 * 2. Mutable/reuse: Use Set*() methods to modify in place
 *
 * Thread-safety is the user's responsibility (don't share and mutate).
 */
class Selection
{
public:
    //========================================================================
    // Factory methods (create new Selection)
    //========================================================================

    /**
     * Create a bounding box selection (hyperslab).
     * @param start  Starting offsets in each dimension
     * @param count  Number of elements in each dimension
     */
    static Selection BoundingBox(const Dims &start, const Dims &count);
    static Selection BoundingBox(const Box<Dims> &box);

    /**
     * Create a block selection for LocalArray variables.
     * @param blockID  Block index from the writing rank
     */
    static Selection Block(size_t blockID);

    //========================================================================
    // Mutating setters (modify in place, return *this for chaining)
    //========================================================================

    Selection &SetBoundingBox(const Dims &start, const Dims &count);
    Selection &SetBoundingBox(const Box<Dims> &box);

    Selection &SetBlock(size_t blockID);

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

    Selection WithSteps(size_t stepStart, size_t stepCount) const;
    Selection WithSteps(const Box<size_t> &steps) const;

    Selection WithMemory(const Dims &memoryStart, const Dims &memoryCount) const;
    Selection WithMemory(const Box<Dims> &memory) const;

    Selection WithAccuracy(const Accuracy &accuracy) const;
    Selection WithAccuracy(double error, double norm = L2_norm, bool relative = false) const;

    /** Access to internal implementation (for Engine use) */
    const core::Selection &GetCoreSelection() const;

private:
    friend class Engine;
    Selection();

    class Impl;
    std::shared_ptr<Impl> m_Impl;
};

} // end namespace adios2

#endif // ADIOS2_BINDINGS_CXX_CXX_SELECTION_H_
