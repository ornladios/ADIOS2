/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_CORE_GETCONTEXT_H_
#define ADIOS2_CORE_GETCONTEXT_H_

namespace adios2
{
namespace core
{

// Opaque per-caller state for thread-safe Get/PerformGets.
class GetContext
{
public:
    virtual ~GetContext() = default;
};

} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_CORE_GETCONTEXT_H_ */
