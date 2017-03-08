/*
 * ADIOS_CPP.h
 *
 *  Created on: Jan 9, 2017
 *      Author: wfg
 */

#ifndef ADIOS_CPP_H_
#define ADIOS_CPP_H_


#include "ADIOS.h"
#include "core/Method.h"

#include "core/Engine.h"
#include "core/Transform.h"
#include "engine/bp/BPFileWriter.h"

//Will allow to create engines directly (no polymorphism)
#ifdef HAVE_DATAMAN
#include "engine/dataman/DataManWriter.h"
#include "engine/dataman/DataManReader.h"
#endif

#include "transform/BZip2.h"


#endif /* ADIOS_CPP_H_ */
