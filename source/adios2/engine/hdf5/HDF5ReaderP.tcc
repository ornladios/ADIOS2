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
    UseHDFRead(variable, data, h5Type);
}

template <class T>
void HDF5ReaderP::GetDeferredCommon(Variable<T> &variable, T *data)
{
    // returns immediately
    // m_HDF53Deserializer.GetDeferredVariable(variable, data);

    throw std::runtime_error("Todo: GetDefCommon");
}

} // end namespace adios2

#endif /* ADIOS2_ENGINE_HDF5_HDF5FILEREADER_TCC_ */
