/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BPFileWriter.tcc implementation of template functions with known type
 *
 *  Created on: May 22, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */
#ifndef ADIOS2_ENGINE_BP_BPFILEWRITER_TCC_
#define ADIOS2_ENGINE_BP_BPFILEWRITER_TCC_

#include "BPFileWriter.h"

namespace adios2
{

template <class T>
void BPFileWriter::PutSyncCommon(Variable<T> &variable, const T *values)
{
    // set variable
    variable.SetData(values);

    m_WrittenVariables.insert(variable.m_Name);

    // if first timestep Write create a new pg index
    if (!m_BP3Serializer.m_MetadataSet.DataPGIsOpen)
    {
        m_BP3Serializer.PutProcessGroupIndex(
            m_IO.m_HostLanguage, m_FileManager.GetTransportsTypes());
    }

    format::BP3Base::ResizeResult resizeResult =
        m_BP3Serializer.ResizeBuffer(variable);

    if (resizeResult == format::BP3Base::ResizeResult::Flush)
    {
        m_BP3Serializer.SerializeData(m_IO);
        auto &buffer = m_BP3Serializer.m_Data.m_Buffer;
        auto &position = m_BP3Serializer.m_Data.m_Position;

        m_FileManager.WriteFiles(buffer.data(), position);
        // set relative position to zero
        position = 0;
        // reset buffer to zero values to current size
        buffer.assign(buffer.size(), '\0');

        // new group index
        m_BP3Serializer.PutProcessGroupIndex(
            m_IO.m_HostLanguage, m_FileManager.GetTransportsTypes());
    }

    // WRITE INDEX to data buffer and metadata structure (in memory)//
    m_BP3Serializer.PutVariableMetadata(variable);
    m_BP3Serializer.PutVariablePayload(variable);

    variable.SetData(nullptr); // not needed after PutSync
}

} // end namespace adios2

#endif /* ADIOS2_ENGINE_BP_BPFILEWRITER_TCC_ */
