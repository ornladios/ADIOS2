/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_TOOLKIT_SHM_SERIALIZEPROCESSES_H_
#define ADIOS2_TOOLKIT_SHM_SERIALIZEPROCESSES_H_

#include "adios2/common/ADIOSConfig.h"
#include "adios2/helper/adiosComm.h"

#include <atomic>
#include <chrono>
#include <thread>

namespace adios2
{
namespace shm
{

class SerializeProcesses
{

public:
    SerializeProcesses(helper::Comm *comm);

    ~SerializeProcesses();

    void Wait();     // blocking wait until it's my turn
    bool IsMyTurn(); // non-blocking check if it's my turn
    void Done();     // this process is done, next please

private:
    helper::Comm *m_NodeComm;
    const int m_Rank;
    const int m_nProc;

    helper::Comm::Win m_Win;
    int *m_ShmValue; // single integer is the whole shm
};

} // end namespace shm
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_SHM_SERIALIZEPROCESSES_H_ */
