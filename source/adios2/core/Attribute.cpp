/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Attribute.h"
#include "Attribute.tcc"

#include "adios2/common/ADIOSMacros.h"

namespace adios2
{
namespace core
{

#define declare_template_instantiation(T) template class Attribute<T>;
ADIOS2_FOREACH_ATTRIBUTE_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace core
} // end namespace adios2
