/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 *  Created on: May 17, 2017
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 *              William F Godoy godoywf@ornl.gov
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

#ifndef ADIOS2_CORE_SELECTION_H_
#define ADIOS2_CORE_SELECTION_H_

#include "adios2/ADIOSConfig.h"
#include "adios2/ADIOSTypes.h"

namespace adios
{
/** Base class for Selection (query) types */
class Selection
{
public:
    /** from derived class */
    const SelectionType m_Type;

    /**
     * Unique constructor
     * @param type derived class type
     */
    Selection(const SelectionType type, const bool debugMode = false);

    virtual ~Selection() = default;

protected:
    /** true: extra checks (recommended) */
    const bool m_DebugMode;
};

} // namespace adios

#endif /* ADIOS2_CORE_SELECTION_H_ */
