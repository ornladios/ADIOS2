/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#ifndef ADIOS2_H_
#define ADIOS2_H_

#include "adios2/ADIOSConfig.h"

#if __cplusplus >= 201103L
#include "adios2/ADIOSTypes.h"
#include "adios2/core/ADIOS.h"
#include "adios2/core/Engine.h"
#include "adios2/core/IO.h"
#include "adios2/core/Operator.h"
#include "adios2/highlevelapi/fstream/ADIOS2fstream.h"
#else
#include "adios2_cxx03.h"
#endif

#endif /* ADIOS2_H_ */
