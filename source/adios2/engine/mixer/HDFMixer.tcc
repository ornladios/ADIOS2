/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * HDFMixer.tcc implementation of template functions with known type
 *
 *  Created on: Aug 16 2017
 *      Author: Junmin GU
 */

#include "HDFMixer.h"

namespace adios2
{
namespace core
{
namespace engine
{

template <class T>
void HDFMixer::DoPutSyncCommon(Variable<T> &variable, const T *values)
{
    // set values
    variable.SetData(values);
    // m_WrittenVariables.insert(variable.m_Name);
    Variable<T> local(variable.m_Name, {}, {}, variable.m_Count,
                      variable.IsConstantDims(), false);

    //    Variable<T> local(variable.m_Name, {}, {}, variable.m_Count,
    //                      variable.m_Count.size(), NULL, false);

    // m_HDFSerialWriter.m_H5File.Write(variable, values);
    // writes only the m_Count() part
    int nDims = std::max(variable.m_Shape.size(), variable.m_Count.size());
    if (nDims == 0)
    {
        // this is scalar
        if (m_HDFVDSWriter.m_Rank == 0)
        {
            m_HDFVDSWriter.m_VDSFile.Write(local, values);
        }
    }
    else
    {
        m_HDFSerialWriter.m_H5File.Write(local, values);
        // std::cout<<"   ==> "<< variable.m_Name<<std::endl;
        // BuildVDS(AddExtension(m_Name, ".h5"), variable,
        // m_HDFSerialWriter.m_H5File.GetHDF5Type<T>());
        m_HDFVDSWriter.AddVar(variable,
                              m_HDFSerialWriter.m_H5File.GetHDF5Type<T>());
    }
}

} // end namespace engine
} // end namespace core
} // namespace adios2
