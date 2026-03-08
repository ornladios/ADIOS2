/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "CudaRoutines.h"

__global__ void __cuda_increment(int offset, float *vec, float val)
{
    vec[blockIdx.x + offset] += val;
}

void cuda_increment(int M, int N, int offset, float *vec, float val)
{
    __cuda_increment<<<M, N>>>(offset, vec, val);
}
