# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

# Client maintainer: vicente.bolea@kitware.com

set(CTEST_TEST_ARGS
  PARALLEL_LEVEL 8
  EXCLUDE ".*/BPWRCUDA.ADIOS2BPCUDAWrong/.*BP4.Serial|.*/BPWRCUDA.ADIOS2BPCUDAMemSel/.*BP4.Serial|Engine.Staging.TestThreads.*|Install.*"
  )
