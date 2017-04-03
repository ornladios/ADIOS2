/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Attribute.h
 *
 *  Created on: Oct 5, 2016
 *      Author: wfg
 */

#ifndef ATTRIBUTE_H_
#define ATTRIBUTE_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <string>
/// \endcond

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

#endif /* ATTRIBUTE_H_ */
