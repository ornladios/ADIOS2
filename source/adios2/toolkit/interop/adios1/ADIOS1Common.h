/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS1Common.h
 *
 *  Created on: Jun 1, 2017
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 *              William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_INTEROP_ADIOS1_ADIOS1COMMON_H_
#define ADIOS2_TOOLKIT_INTEROP_ADIOS1_ADIOS1COMMON_H_

#include <adios_types.h>

#include "adios2/ADIOSMPICommOnly.h"
#include "adios2/ADIOSMacros.h"
#include "adios2/ADIOSTypes.h"

namespace adios2
{
namespace interop
{

class ADIOS1Common
{
public:
    /**Save file name from constructor for Advance when re-open in ADIOS1 */
    const std::string m_FileName;
    const std::string m_OpenModeString;
    bool m_IsInitialized = false; ///< set to true after calling adios_init()
    bool m_IsFileOpen = false;
    int m_VerboseLevel = 0;

    ADIOS1Common(const std::string &fileName, MPI_Comm mpiComm,
                 const bool debugMode);
    ~ADIOS1Common();

    bool CheckADIOS1TypeCompatibility(const std::string &name,
                                      std::string adios2Type,
                                      enum ADIOS_DATATYPES adios1Type);

protected:
    MPI_Comm m_MPIComm;
    const bool m_DebugMode = false;

    template <class T>
    enum ADIOS_DATATYPES GetADIOS1Type() const;

    std::string DimsToCSVLocalAware(const Dims &dims);
};

} // end namespace interop
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_INTEROP_ADIOS1_ADIOS1COMMON_H_ */
