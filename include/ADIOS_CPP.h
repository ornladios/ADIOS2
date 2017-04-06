/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS_CPP.h
 *
 *  Created on: Jan 9, 2017
 *      Author: wfg
 */

#ifndef ADIOS_CPP_H_
#define ADIOS_CPP_H_

#include "ADIOS.h"
#include "ADIOSTypes.h"
#include "core/Method.h"

#include "core/Engine.h"
#include "core/Transform.h"
#include "engine/bp/BPFileWriter.h"

// Will allow to create engines directly (no polymorphism)
#ifdef ADIOS_HAVE_DATAMAN
#include "engine/dataman/DataManReader.h"
#include "engine/dataman/DataManWriter.h"
#endif

#ifdef ADIOS_HAVE_BZIP2
#include "transform/BZip2.h"
#endif

#endif /* ADIOS_CPP_H_ */
