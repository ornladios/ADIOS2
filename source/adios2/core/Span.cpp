/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Span.tcc"

namespace adios2
{
namespace core
{

#define declare_template_instantiation(T) template class Span<T>;
ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_type

} // end namespace core
} // end namespace adios2
