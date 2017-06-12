/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOSPy.h  python binding to ADIOS class
 *
 *  Created on: Mar 13, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_PYTHON_SOURCE_ADIOSPY_H_
#define ADIOS2_BINDINGS_PYTHON_SOURCE_ADIOSPY_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <string>
/// \endcond

#include "IOPy.h"

namespace adios
{

class ADIOSPy
{

public:
    ADIOSPy(MPI_Comm mpiComm, const bool debug = false);
    ~ADIOSPy() = default;

    IOPy DeclareIO(const std::string name);

private:
    const bool m_DebugMode;
    adios::ADIOS m_ADIOS;
};

} // end namespace adios

#endif /* BINDINGS_PYTHON_SOURCE_ADIOSPY_H_ */
