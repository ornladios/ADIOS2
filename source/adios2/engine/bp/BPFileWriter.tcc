/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BPFileWriter.tcc implementation of template functions with known type
 *
 *  Created on: May 22, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "BPFileWriter.h"

namespace adios
{

template <class T>
void BPFileWriter::DoWriteCommon(Variable<T> &variable, const T *values)
{
    // set variable
    variable.m_AppValues = values;
    m_WrittenVariables.insert(variable.m_Name);

    // if first timestep Write create a new pg index
    if (m_BP1Writer.m_MetadataSet.DataPGIsOpen == false)
    {
        m_BP1Writer.WriteProcessGroupIndex(
            m_IO.m_HostLanguage, m_TransportsManager.GetTransportsTypes());
    }

    // pre-calculate new metadata and payload sizes
    //        m_TransportFlush = CheckBufferAllocation(
    //        m_BP1Writer.GetVariableIndexSize( variable ) +
    //        variable.PayLoadSize(),
    //                                                  m_GrowthFactor,
    //                                                  m_MaxBufferSize,
    //                                                  m_Buffer.m_Data );

    // WRITE INDEX to data buffer and metadata structure (in memory)//
    m_BP1Writer.WriteVariableMetadata(variable);

    if (m_TransportFlush == true) // in batches
    {
        // flatten data

        // flush to transports

        // reset relative positions to zero, update absolute position
    }
    else // Write data to buffer
    {
        m_BP1Writer.WriteVariablePayload(variable);
    }
    variable.m_AppValues = nullptr; // not needed after write
}

} // end namespace adios
