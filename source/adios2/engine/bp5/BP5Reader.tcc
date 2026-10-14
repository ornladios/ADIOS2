/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_ENGINE_BP5_BP5READER_TCC_
#define ADIOS2_ENGINE_BP5_BP5READER_TCC_

#include "BP5Reader.h"

#include "adios2/common/Selection.h"
#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace core
{
namespace engine
{

inline void BP5Reader::GetSyncCommon(VariableBase &variable, void *data)
{
    GetSyncCommon(variable, data, InferSelection(variable));
}

void BP5Reader::GetDeferredCommon(VariableBase &variable, void *data)
{
    GetDeferredCommon(variable, data, InferSelection(variable));
}

inline void BP5Reader::GetSyncCommon(VariableBase &variable, void *data, const Selection &selection)
{
    bool need_sync = Deserializer("Get").QueueGet(variable, data, selection, m_dataIsRemote);
    if (need_sync)
        PerformGets();
}

void BP5Reader::GetDeferredCommon(VariableBase &variable, void *data, const Selection &selection)
{
    (void)Deserializer("Get").QueueGet(variable, data, selection, m_dataIsRemote);
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_BP5_BP5READER_TCC_ */
