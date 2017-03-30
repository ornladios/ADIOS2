/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS_Types.h
 *
 *  Created on: Mar 23, 2017
 *      Author: pnb
 */

#ifndef ADIOS_TYPES_H_
#define ADIOS_TYPES_H_

namespace adios
{

/** Use these values in Dims() when defining variables
 */
enum
{
  VARYING_DIMENSION = -1, //!< VARYING_DIMENSION
  LOCAL_VALUE = 0,        //!< LOCAL_VALUE
  GLOBAL_VALUE = 1        //!< GLOBAL_VALUE
};

enum class Verbose
{
  ERROR = 0,
  WARN = 1,
  INFO = 2,
  DEBUG = 3
};

enum class IOMode
{
  INDEPENDENT = 0,
  COLLECTIVE = 1
};

} // end namespace

#endif /* ADIOS_TYPES_H_ */
