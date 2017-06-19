/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * HDF5CommonP.h
 *
 *  Created on: March 20, 2017
 *      Author: Junmin
 */

#ifndef ADIOS2_TOOLKIT_INTEROP_HDF5_HDF5COMMON_H_
#define ADIOS2_TOOLKIT_INTEROP_HDF5_HDF5COMMON_H_

#include <hdf5.h>

#include <string>

#include "adios2/ADIOSMPICommOnly.h"
#include "adios2/ADIOSMacros.h"
#include "adios2/ADIOSTypes.h"
#include "adios2/core/Variable.h"

#include <stdexcept> // for Intel Compiler

namespace adios2
{
namespace interop
{

class HDF5Common
{

public:
    /**
     * Unique constructor for HDF5 file
     * @param debugMode true: extra exception checks
     */
    HDF5Common(const bool debugMode);

    void Init(const std::string &name, MPI_Comm comm, bool toWrite);

    template <class T>
    void Write(Variable<T> &variable, const T *values);

    void Close();
    void Advance();

    unsigned int GetNumTimeSteps();
    void WriteTimeSteps();

    hid_t m_PropertyListId = -1;
    hid_t m_FileId = -1;
    hid_t m_GroupId = -1;

    hid_t m_DefH5TypeComplexDouble;
    hid_t m_DefH5TypeComplexFloat;
    hid_t m_DefH5TypeComplexLongDouble;

    unsigned int m_CurrentTimeStep = 0;

    void CheckWriteGroup();

    template <class T>
    hid_t GetHDF5Type(); // should this be public?

private:
    const bool m_DebugMode;
    bool m_WriteMode = false;
    unsigned int m_NumTimeSteps = 0;
};

// Explicit declaration of the public template methods
#define declare_template_instantiation(T)                                      \
    extern template void HDF5Common::Write(Variable<T> &variable,              \
                                           const T *value);

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace interop
} // end namespace adios

#endif /* ADIOS2_TOOLKIT_INTEROP_HDF5_HDF5COMMON_H_ */
