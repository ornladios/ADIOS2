/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_ENGINE_NULLREADER_TCC_
#define ADIOS2_ENGINE_NULLREADER_TCC_

#include "NullReader.h"

namespace adios2
{
namespace core
{
namespace engine
{

template <>
inline void NullReader::GetSyncCommon(Variable<std::string> &variable, std::string *data)
{
    variable.m_Data = data;
}

template <class T>
inline void NullReader::GetSyncCommon(Variable<T> &variable, T *data)
{
    variable.m_Data = data;
}

template <class T>
void NullReader::GetDeferredCommon(Variable<T> &variable, T *data)
{
    variable.m_Data = data;
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_NULLREADER_TCC_ */
