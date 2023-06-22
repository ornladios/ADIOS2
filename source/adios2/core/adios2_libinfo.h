/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_libinfo.h
 *
 *  Created on: June 22, 2023
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#ifndef ADIOS2_LIBINFO_H_
#define ADIOS2_LIBINFO_H_

#ifdef __cplusplus
extern "C" {
#endif

extern const int adios2_version_major;
extern const int adios2_version_minor;
extern const int adios2_version_patch;
extern const char *const adios2_version_str;

/** Return the list of available Engines in the installed adios2 library */
void adios2_available_engines(int *nentries, char const ***list);

/** Return the list of available Engines in the installed adios2 library */
void adios2_available_operators(int *nentries, char const ***list);

/** Return the list of available features in the installed adios2 library */
void adios2_available_features(int *nentries, char const ***list);

/** Free function for list returned by adios2_available_engines() and
 * adios2_available_operators()
 */
void adios2_free_list(int nentries, char const ***list);

#ifdef __cplusplus
} // end extern C
#endif

#endif /* ADIOS2_LIBINFO_H_ */
