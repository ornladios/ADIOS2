/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */


// Test for CXX11 deprecated headers
#include <adios2/cxx11/ADIOS.h>
// This is defined in 'adios2/cxx/ADIOS.h'
#ifndef ADIOS2_BINDINGS_CXX_CXX_ADIOS_H_
#error "failed to load 'adios2/cxx11/ADIOS.h'"
#endif

#include <adios2.h>

#if ADIOS2_USE_MPI
#error "ADIOS2_USE_MPI is true for source not using ADIOS2 MPI bindings"
#endif

int main(void)
{
    adios2::ADIOS adios;
    return 0;
}
