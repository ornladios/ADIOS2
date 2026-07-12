/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_HELPER_ADIOSSYSTEM_H_
#define ADIOS2_HELPER_ADIOSSYSTEM_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <string>
#include <vector>
/// \endcond

#include "adios2/common/ADIOSTypes.h"
#include "adios2/core/IO.h"
#include "adios2/helper/adiosComm.h"

namespace adios2
{
namespace helper
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

/**
 * Support for language bindings, identify if data is row-major (C, C++) or not
 * (Fortran, R)
 * @param hostLanguage input host language
 * @return true: row-major, false: column-major
 */
bool IsRowMajor(const std::string hostLanguage) noexcept;

/**
 * Support for language bindings, identify if data is zero-indexed (C, C++) or
 * not (Fortran, R)
 * @param hostLanguage input host language
 * @return true: zero-indexed, false: one-indexed
 */
bool IsZeroIndexed(const std::string hostLanguage) noexcept;

/**
 * Function to be called inside a catch(...) block to convert C++ exceptions to
 * error integers. Used by C, Fortran bindings.
 * @param function function name to be printed in std::cerr
 * @return error integers mapped to exceptions:
 * <pre>
 * 	none                   : 0
 *  std::invalid_argument  : 1
 *  std::system_error      : 2
 *  std::runtime_exception : 3
 *  std::exception         : 4
 * </pre>
 */
int ExceptionToError(const std::string &function);

bool IsHDF5File(const std::string &name, core::IO &io, helper::Comm &comm,
                const std::vector<Params> &transportsParameters) noexcept;
/** Rank-local IsHDF5File: no internal broadcast; call from a single rank. */
bool IsHDF5FileLocal(const std::string &name, core::IO &io, helper::Comm &comm,
                     const std::vector<Params> &transportsParameters) noexcept;
/** Rank-local BP version sniff: reads the version byte from the dataset's
 *  md.idx; returns '3'/'4'/'5', or '5' when the index is absent/unreadable. */
char BPVersionLocal(const std::string &name) noexcept;

/** Return true if 'name' is a dataset directory written by the DAOS
 *  engine.  Identified by the data_oids.txt index file the DAOS engine
 *  produces (mapping writer ranks to DAOS array object IDs); no
 *  BP3/BP4/BP5 dataset contains this file.  Used by IO::Open to
 *  auto-detect the DAOS engine -- a DAOS dataset directory also carries
 *  an mmd.0 and would otherwise mis-detect as BP5.
 */
bool IsDAOSDataset(const std::string &name) noexcept;

/** Return the number of available hardware threads on the node.
 * It might return 0 if the detection does not work
 */
unsigned int NumHardwareThreadsPerNode();

/** Attempt to raise the limit of number of files opened at once
 *  Return: the limit on number of open files
 */
size_t RaiseLimitNoFile();

/**
 * Remove stale BP files from a directory before writing.
 * Scans directory for all files and removes any not in filesToKeep.
 * Only rank 0 performs operations; all ranks synchronize via barrier.
 *
 * @param directory BP directory path
 * @param filesToKeep List of files the engine will create (basenames only)
 * @param comm MPI communicator
 */
void CleanupBPDirectory(const std::string &directory, const std::vector<std::string> &filesToKeep,
                        helper::Comm &comm);

} // end namespace helper
} // end namespace adios2

#endif /* ADIOS2_HELPER_ADIOSSYSTEM_H_ */
