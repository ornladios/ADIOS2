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

#include "IOPy.h"

/// \cond EXCLUDE_FROM_DOXYGEN
#include <memory> //std::shared_ptr
#include <string>
/// \endcond

#include "adios2/ADIOSMPICommOnly.h"
#include "adios2/core/ADIOS.h"

namespace adios2
{

class ADIOSPy
{

public:
    ADIOSPy(const std::string configFile, MPI_Comm mpiComm,
            const bool debugMode);
    ADIOSPy(MPI_Comm mpiComm, const bool debugMode);
    ADIOSPy(const std::string configFile, const bool debugMode);
    ADIOSPy(const bool debugMode);

    ~ADIOSPy() = default;

    IOPy DeclareIO(const std::string name);
    IOPy AtIO(const std::string name);

    void FlushAll();

private:
    const bool m_DebugMode = true;
    std::shared_ptr<adios2::ADIOS> m_ADIOS;
};

} // end namespace adios2

#endif /* BINDINGS_PYTHON_SOURCE_ADIOSPY_H_ */
