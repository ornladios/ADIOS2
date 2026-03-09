/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2EXAMPLES_DECOMP_H
#define ADIOS2EXAMPLES_DECOMP_H

#include <stddef.h>

extern size_t get_random(int minv, int maxv, int rank);
extern void gather_decomp_1d(size_t *, size_t *, size_t *);
extern void decomp_1d(size_t *, size_t *, size_t *);
#endif // ADIOS2EXAMPLES_DECOMP_H
