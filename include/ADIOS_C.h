/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS_C.h  "C" interface to ADIOS C++ class. This header defines the C API
 *
 *  Created on: Oct 31, 2016
 *      Author: wfg
 */

#ifndef ADIOS_C_H_
#define ADIOS_C_H_

#ifdef ADIOS_NOMPI
#define MPI_Comm int
#else
#include <mpi.h>
#endif

typedef void ADIOS;
typedef void Method;
typedef void Engine;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ADIOS Init
 * @param mpicomm MPI communicator from user app
 */
void adios_init(MPI_Comm mpiComm);

/**
 * ADIOS Init in debug mode: extra checks
 * @param mpicomm MPI communicator from user app
 */
void adios_init_debug(MPI_Comm mpiComm);

/**
 * Sequential version (nompi)
 */
void adios_init_nompi();

/**
 *
 * @param fileName
 * @param accessMode
 * @param mpiComm
 * @return engine handler
 */
int adios_open(const char *fileName, const char *accessMode, MPI_Comm mpiComm);

/**
 *
 * @param variableName
 * @param values
 */
void adios_write(const char *variableName, const void *values);

/**
 *
 * @param handler
 */
void adios_close(const int handler);

void adios_finalize(const ADIOS *adiosC); // deallocate ADIOS pointer

void adios_monitor_groups(const ADIOS *adiosC);

void adios_monitor_groups_file(const ADIOS *adiosC, const char *fileName,
                               const char *mode);

#ifdef __cplusplus
} // end extern C
#endif

#ifdef ADIOS_NOMPI
#undef MPI_Comm
#endif

#endif /* ADIOS_C_H_ */
