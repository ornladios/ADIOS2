/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * HDF5ReaderP.tcc
 *
 *  Created on: Oct 30, 2017
 *      Author: jgu@lbl.gov
 */

#ifndef ADIOS2_ENGINE_HDF5_HDF5FILEREADER_TCC_
#define ADIOS2_ENGINE_HDF5_HDF5FILEREADER_TCC_

#include "HDF5ReaderP.h"

namespace adios2
{
namespace core
{
namespace engine
{

template <class T>
void HDF5ReaderP::GetSyncCommon(Variable<T> &variable, T *data)
{
    // subfile info
    /*  no good way to check it is not reference to null
        if (&variable  == nullptr) {
           return;
        }
    */
    hid_t h5Type = m_H5File.GetHDF5Type<T>();
    //    UseHDFRead(variable.m_Name, data, h5Type);
    if (m_InStreamMode)
    {
        variable.m_StepsStart = m_StreamAt;
        variable.m_StepsCount = 1;
    }
    UseHDFRead(variable, data, h5Type);
}

template <class T>
void HDF5ReaderP::GetDeferredCommon(Variable<T> &variable, T *data)
{
#ifdef NEVER
    // returns immediately
    // m_HDF53Deserializer.GetDeferredVariable(variable, data);

    if (m_InStreamMode)
    {
        variable.m_StepsStart = m_StreamAt; // current step
        variable.m_StepsCount = 1;
    }
    hid_t h5Type = m_H5File.GetHDF5Type<T>();
    UseHDFRead(variable, data, h5Type);
#else
    m_DeferredStack.push_back(variable.m_Name);
    variable.SetData(data);
#endif
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_HDF5_HDF5FILEREADER_TCC_ */
