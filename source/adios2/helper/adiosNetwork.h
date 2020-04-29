/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosNetwork.h
 * Common Network functions needed by staging and streaming engines
 *
 *  Created on: March 22, 2019
 *      Author: Jason Wang
 */

#ifndef ADIOS2_HELPER_ADIOSNETWORK_H_
#define ADIOS2_HELPER_ADIOSNETWORK_H_

#ifndef _WIN32
#if defined(ADIOS2_HAVE_TABLE)

/// \cond EXCLUDE_FROM_DOXYGEN
#include <string>
#include <vector>
/// \endcond

namespace adios2
{
namespace helper
{

class Comm;

/**
 * returns a vector of strings with all available IP addresses on the node
 * @return vector of strings
 */
std::vector<std::string> AvailableIpAddresses() noexcept;

} // end namespace helper
} // end namespace adios2

#endif // ADIOS2_HAVE_TABLE
#endif // _WIN32
#endif // ADIOS2_HELPER_ADIOSNETWORK_H_
