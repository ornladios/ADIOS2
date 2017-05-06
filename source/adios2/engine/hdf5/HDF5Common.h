/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * HDF5CommonP.h
 *
 *  Created on: March 20, 2017
 *      Author: Junmin
 */

#ifndef ADIOS2_ENGINE_HDF5_HDF5COMMON_P_H_
#define ADIOS2_ENGINE_HDF5_HDF5COMMON_P_H_

#include <string>

#include "adios2/ADIOSMPICommOnly.h"

#include <hdf5.h>

namespace adios
{

class HDF5Common
{

public:
    /**
     * Constructor for HDF5 file
     */
    HDF5Common();

    void Init(const std::string name, MPI_Comm comm, bool toWrite);
    void Close();
    void Advance();

    unsigned int GetNumTimeSteps();
    void WriteTimeSteps();

    hid_t m_PropertyListId, m_FileId;
    hid_t m_GroupId;

    hid_t m_DefH5TypeComplexDouble;
    hid_t m_DefH5TypeComplexFloat;
    hid_t m_DefH5TypeComplexLongDouble;

    unsigned int m_CurrentTimeStep;

    void CheckWriteGroup();

private:
    bool m_WriteMode;
    unsigned int m_NumTimeSteps;
};

} // end namespace adios

#endif /* ADIOS2_ENGINE_HDF5_HDF5COMMON_P_H_ */
