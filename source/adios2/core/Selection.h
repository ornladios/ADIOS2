/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

/*
 *   Selection API in C++
 *
 *   A SELECTION is the data ranges resulting from a QUERY over a file and
 * variable(s).
 *
 *   A SELECTION can be a list of bounding boxes and point-sets. Other data
 * structures
 *   are not supported. Any query will result in such a selection.
 *   Other selections are the write-block and auto.
 *
 *   Write block is a block of data written
 *   by a writer process, it is identified by an index. If each writer outputs
 * one block
 *   then the index equals to the rank of the write process. With multi-var
 * writing and
 *   multiple steps in a file, index runs from 0 as rank 0 process' first block.
 *
 *   Auto selection lets the read method to automatically choose what to return.
 * It will
 *   be a block / blocks of selected writers.
 *
 *   If a query is a simple bounding box, the selection is the bounding box
 * itself, so
 *   the application does not need to retrieve the selection to work on the read
 * data.
 */
#ifndef __ADIOS_SELECTION_H__
#define __ADIOS_SELECTION_H__

#include "ADIOSConfig.h"

namespace adios
{

/* Type of selection */
enum class SelectionType
{
    // Contiguous block of data defined by offsets and counts in each
    // dimension
    BoundingBox,

    // List of individual points
    Points,

    // Selection of an individual block written by a writer process
    WriteBlock,

    // Let the method decide what to return
    Auto
};

class Selection
{
public:
    Selection(const SelectionType t) : m_Type(t){};
    const SelectionType m_Type;
};

} // namespace adios

#endif /*__ADIOS_SELECTION_H__*/
