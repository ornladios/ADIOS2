/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_TOOLKIT_SHM_SPINLOCK_H_
#define ADIOS2_TOOLKIT_SHM_SPINLOCK_H_

#include <atomic>

namespace adios2
{
namespace shm
{

class Spinlock
{
    /* from
     * https://wang-yimu.com/a-tutorial-on-shared-memory-inter-process-communication
     */
public:
    Spinlock();
    virtual ~Spinlock() = default;
    void lock();
    void unlock();

private:
    inline bool try_lock();
    std::atomic_flag flag_; //{ATOMIC_FLAG_INIT};
};

} // end namespace shm
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_SHM_SPINLOCK_H_ */
