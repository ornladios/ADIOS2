/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Attribute.h
 *
 *  Created on: Oct 5, 2016
 *      Author: wfg
 */

#ifndef ADIOS2_CORE_ATTRIBUTE_H_
#define ADIOS2_CORE_ATTRIBUTE_H_

#include <string>

#include "adios2/ADIOSConfig.h"

namespace adios
{

/**
 * Plain-old data struct that defines an attribute in an ADIOS group in Group.h
 */
struct Attribute
{
    const char TypeID;       ///< '0': string, '1': numeric
    const std::string Value; ///< information about the attribute
};

} // end namespace

#endif /* ADIOS2_CORE_ATTRIBUTE_H_ */
