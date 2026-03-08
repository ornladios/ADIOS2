/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef WITH_ADIOS2
#include <adios2.h>
#endif

#include "foo.h"

void foo(void)
{
#ifdef WITH_ADIOS2
    adios2::ADIOS ctx;
#endif
}
