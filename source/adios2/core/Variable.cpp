/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Variable.cpp : needed for template separation using Variable.tcc
 *
 *  Created on: Jun 8, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "Variable.h"
#include "Variable.tcc"

#include "adios2/ADIOSMacros.h"
#include "adios2/core/Engine.h"
#include "adios2/helper/adiosFunctions.h" //helper::GetType<T>

namespace adios2
{
namespace core
{

#define declare_template_instantiation(T) template class Variable<T>;
ADIOS2_FOREACH_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace core
} // end namespace adios2
