/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Spinlock.h"

#include <chrono>
#include <thread>

namespace adios2
{
namespace shm
{

Spinlock::Spinlock() { flag_.clear(); }
void Spinlock::lock()
{
    while (!try_lock())
    {
        std::this_thread::sleep_for(std::chrono::duration<double>(0.00001));
    }
}
void Spinlock::unlock() { flag_.clear(); }

inline bool Spinlock::try_lock() { return !flag_.test_and_set(); }

} // end namespace shm
} // end namespace adios2
