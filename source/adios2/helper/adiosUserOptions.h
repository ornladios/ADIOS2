/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosUserOptions.h YAML file parsing functionality for ~/.config/adios2/adios2.yaml
 */

#ifndef ADIOS2_HELPER_ADIOSUSEROPTIONS_H_
#define ADIOS2_HELPER_ADIOSUSEROPTIONS_H_

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

void ParseUserOptionsFile(Comm &comm, const std::string &configFileYAML, UserOptions &options,
                          std::string &homePath);

} // end namespace helper
} // end namespace adios2

#endif /* ADIOS2_HELPER_ADIOSUSEROPTIONS_H_ */
