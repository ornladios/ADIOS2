/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP5Reader.tcc
 *
 *  Created on: Aug 1, 2018
 *      Author: Lipeng Wan wanl@ornl.gov
 */

#ifndef ADIOS2_ENGINE_BP5_BP5READER_TCC_
#define ADIOS2_ENGINE_BP5_BP5READER_TCC_

#include "BP5Reader.h"

#include "adios2/core/Selection.h"
#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace core
{
namespace engine
{

inline void BP5Reader::GetSyncCommon(VariableBase &variable, void *data)
{
    auto sel = InferSelection(variable);
    bool need_sync = m_BP5Deserializer->QueueGet(variable, data, sel, m_dataIsRemote);
    if (need_sync)
        PerformGets();
}

void BP5Reader::GetDeferredCommon(VariableBase &variable, void *data)
{
    auto sel = InferSelection(variable);
    (void)m_BP5Deserializer->QueueGet(variable, data, sel, m_dataIsRemote);
}

inline void BP5Reader::GetSyncCommon(VariableBase &variable, void *data, const Selection &selection)
{
    bool need_sync = m_BP5Deserializer->QueueGet(variable, data, selection, m_dataIsRemote);
    if (need_sync)
        PerformGets();
}

void BP5Reader::GetDeferredCommon(VariableBase &variable, void *data, const Selection &selection)
{
    (void)m_BP5Deserializer->QueueGet(variable, data, selection, m_dataIsRemote);
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_BP5_BP5READER_TCC_ */
