/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosINI.h basic INI file parsing functionality
 */

#ifndef ADIOS2_HELPER_ADIOSINI_H_
#define ADIOS2_HELPER_ADIOSINI_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <map>
#include <string>
/// \endcond

#include "adios2/common/ADIOSTypes.h"
#include "adios2/helper/adiosComm.h"

namespace adios2
{
namespace helper
{

void ParseINIFile(Comm &comm, const std::string &configFileINI, INIOptions &options,
                  std::string &homePath);

} // end namespace helper
} // end namespace adios2

#endif /* ADIOS2_HELPER_ADIOSINI_H_ */
