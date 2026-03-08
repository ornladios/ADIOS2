/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_HELPER_ADIOSGPUFUNCTIONS_H_
#define ADIOS2_HELPER_ADIOSGPUFUNCTIONS_H_

#ifdef ADIOS2_HAVE_CUDA
#include "adios2/helper/adiosCUDA.h"
#endif

#ifdef ADIOS2_HAVE_KOKKOS
#include "adios2/helper/kokkos/adiosKokkos.h"
#endif

#endif /* ADIOS2_HELPER_ADIOSGPUFUNCTIONS_H_ */
