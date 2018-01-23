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
#include "adios2/core/IO.h" // for CreateVar
#include "adios2/core/Variable.h"

#include <stdexcept> // for Intel Compiler

namespace adios2
{
namespace interop
{

typedef enum {
    E_H5_DATASET = 0,
    E_H5_DATATYPE = 1,
    E_H5_GROUP = 2,
    E_H5_SPACE = 3,
} ADIOS_ENUM_H5;

class HDF5TypeGuard
{
public:
    HDF5TypeGuard(hid_t key, ADIOS_ENUM_H5 type)
    {
        m_Key = key;
        m_Type = type;
        if (key < 0)
        {
            throw std::ios_base::failure("ERROR: HDF5 failure detected.");
        }
    }

    ~HDF5TypeGuard()
    {
        if (m_Type == E_H5_DATASET)
        {
            H5Dclose(m_Key);
        }
        else if (m_Type == E_H5_GROUP)
        {
            H5Gclose(m_Key);
        }
        else if (m_Type == E_H5_SPACE)
        {
            H5Sclose(m_Key);
        }
        else if (m_Type == E_H5_DATATYPE)
        {
            H5Tclose(m_Key);
        }
        else
        {
            printf(" UNABLE to close \n");
        }
    }

private:
    ADIOS_ENUM_H5 m_Type;
    hid_t m_Key;
};

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

    void SetAdiosStep(int ts);

    unsigned int GetNumAdiosSteps();
    void WriteAdiosSteps();

    void ReadVariables(unsigned int ts, IO &io);
    void ReadAllVariables(IO &io);
    void CreateVar(IO &io, hid_t h5Type, std::string const &name);

    template <class T>
    void AddVar(IO &io, std::string const &name, hid_t datasetId);

    static void StaticGetAdiosStepString(std::string &adiosStepName, int ts);

    hid_t m_PropertyListId = -1;
    hid_t m_FileId = -1;
    hid_t m_GroupId = -1;

    hid_t m_DefH5TypeComplexDouble;
    hid_t m_DefH5TypeComplexFloat;
    hid_t m_DefH5TypeComplexLongDouble;

    unsigned int m_CurrentAdiosStep = 0;

    void CheckWriteGroup();

    template <class T>
    hid_t GetHDF5Type(); // should this be public?

    template <class T>
    T GetADIOSType(hid_t);

private:
    const bool m_DebugMode;
    bool m_WriteMode = false;
    unsigned int m_NumAdiosSteps = 0;
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
