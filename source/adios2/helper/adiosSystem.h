/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosSystem.h  system related functions using std or POSIX,
 * we might wait for C++17 filesystem
 *
 *  Created on: May 17, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_HELPER_ADIOSSYSTEM_H_
#define ADIOS2_HELPER_ADIOSSYSTEM_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <string>
#include <vector>
/// \endcond

namespace adios2
{

/**
 * Creates a chain of directories using POSIX systems calls (stat, mkdir),
 * Verifies if directory exists before creating a new one. Permissions are 777
 * for now
 * @param fullPath /full/path/for/directory
 * @return true: directory exists, false: failed to create or access directory
 */
bool CreateDirectory(const std::string &fullPath) noexcept;

/**
 * Check if system is little endian
 * @return true: little endian, false: big endian
 */
bool IsLittleEndian() noexcept;

/**
 * returns a string with current local time and date information from std::ctime
 * @return string from char* std::ctime
 */
std::string LocalTimeDate() noexcept;

} // end namespace adios

#endif /* ADIOS2_HELPER_ADIOSSYSTEM_H_ */
