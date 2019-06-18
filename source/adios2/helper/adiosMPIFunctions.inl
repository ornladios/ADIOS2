/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosMPIFunctions.inl
 *
 *  Created on: Sep 7, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_HELPER_ADIOSMPIFUNCTIONS_INL_
#define ADIOS2_HELPER_ADIOSMPIFUNCTIONS_INL_
#ifndef ADIOS2_HELPER_ADIOSMPIFUNCTIONS_H_
#error "Inline file should only be included from it's header, never on it's own"
#endif

namespace adios2
{
namespace helper
{

template <class T>
std::vector<T> AllGatherValues(const T source, MPI_Comm mpiComm)
{
    int size;
    SMPI_Comm_size(mpiComm, &size);
    std::vector<T> output(size);

    T sourceCopy = source; // so we can have an address for rvalues
    AllGatherArrays(&sourceCopy, 1, output.data(), mpiComm);
    return output;
}

} // end namespace helper
} // end namespace adios2

#endif /* ADIOS2_HELPER_ADIOSMPIFUNCTIONS_INL_ */
