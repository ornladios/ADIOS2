/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Created by Dmitry Ganyushin ganyushindi@ornl.gov
 */

#ifndef ADIOS2EXAMPLES_DECOMP_H
#define ADIOS2EXAMPLES_DECOMP_H

extern long long int get_random(int, int);
extern void gather_decomp_1d(long long int *, long long int *, long long int *);
extern void decomp_1d(long long int, long long int *, long long int *);
#endif // ADIOS2EXAMPLES_DECOMP_H
