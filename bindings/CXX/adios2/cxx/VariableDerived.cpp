/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "VariableDerived.h"

#include "adios2/core/VariableDerived.h"

namespace adios2
{
VariableDerived::VariableDerived(core::VariableDerived *variable) : m_VariableDerived(variable) {}
} // end namespace adios2
