/*
 * ADIOS is freely available under the terms of the BSD license described
 * in the COPYING file in the top level directory of this source distribution.
 *
 * Copyright (c) 2008 - 2009.  UT-BATTELLE, LLC. All rights reserved.
 */

#include "adios2/common/ADIOSConfig.h"
#include "adios2/common/ADIOSMacros.h"
#include "adios2/core/ADIOS.h"
#include "adios2/core/Engine.h"
#include "adios2/core/IO.h"
#include "adios2/core/Variable.h"
#include "adios2/helper/adiosFunctions.h"

#include <map>

namespace adios2
{
namespace utils
{

const size_t MAX_MASKS = 100;

void init_globals();
int compile_regexp_masks(void);
void printSettings(void);
int doSplit();

template <class T>
size_t relative_to_absolute_step(core::Variable<T> *variable,
                                 const size_t relstep);

bool matchesAMask(const char *name);

// close namespace
}
}
