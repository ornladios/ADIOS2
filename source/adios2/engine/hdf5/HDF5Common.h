/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * HDF5CommonP.h
 *
 *  Created on: March 20, 2017
 *      Author: Junmin
 */

#ifndef HDF5_COMMON_P_H_
#define HDF5_COMMON_P_H_

#include "adios2/ADIOSMPICommOnly.h"
#include "adios2/core/Engine.h"

#include <hdf5.h>

namespace adios
{

class HDF5Common
{

public:
    /**
     * Constructor for HDF5 file
     * @param file name
     */
    HDF5Common();

    virtual ~HDF5Common();

    void H5_Init(const std::string name, MPI_Comm m_MPIComm, bool toWrite);
    void H5_Close();
    void H5_Advance(int totalts);

    int GetNumTimeSteps();
    void WriteTimeSteps();

    hid_t _plist_id, _file_id;
    hid_t _group_id;

    hid_t DefH5T_COMPLEX_DOUBLE;
    hid_t DefH5T_COMPLEX_FLOAT;
    hid_t DefH5T_COMPLEX_LongDOUBLE;

    int _currentTimeStep;

    void CheckWriteGroup(); 
private:
    void H5_AdvanceWrite();
    bool _writeMode;
    int _total_timestep;
};

} // end namespace adios

#endif /* HDF5_COMMON_P_H_ */
