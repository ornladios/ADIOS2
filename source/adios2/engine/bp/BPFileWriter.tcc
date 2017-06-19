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

namespace adios2
{

template <class T>
void BPFileWriter::DoWriteCommon(Variable<T> &variable, const T *values)
{
    // set variable
    variable.m_AppValues = values;
    m_WrittenVariables.insert(variable.m_Name);

    // if first timestep Write create a new pg index
    if (!m_BP1Writer.m_MetadataSet.DataPGIsOpen)
    {
        m_BP1Writer.WriteProcessGroupIndex(
            m_IO.m_HostLanguage, m_TransportsManager.GetTransportsTypes());
    }

    const size_t oldSize = m_BP1Writer.m_HeapBuffer.GetDataSize();

    format::BP1Base::ResizeResult resizeResult =
        m_BP1Writer.ResizeBuffer(variable);

    const size_t newSize = m_BP1Writer.m_HeapBuffer.GetDataSize();

    //    if (resizeResult == format::BP1Base::ResizeResult::Success)
    //    {
    //        std::cout << "Old buffer size: " << oldSize << "\n";
    //        std::cout << "New buffer size: " << newSize << "\n";
    //    }

    if (resizeResult == format::BP1Base::ResizeResult::Flush)
    {
        m_BP1Writer.Flush();
        auto &heapBuffer = m_BP1Writer.m_HeapBuffer;

        m_TransportsManager.WriteFiles(heapBuffer.GetData(),
                                       heapBuffer.m_DataPosition);
        // set relative position to zero
        heapBuffer.m_DataPosition = 0;
        // reset buffer to zero values
        heapBuffer.m_Data.assign(heapBuffer.GetDataSize(), '\0');

        m_BP1Writer.WriteProcessGroupIndex(
            m_IO.m_HostLanguage, m_TransportsManager.GetTransportsTypes());
    }

    // WRITE INDEX to data buffer and metadata structure (in memory)//
    m_BP1Writer.WriteVariableMetadata(variable);
    m_BP1Writer.WriteVariablePayload(variable);

    variable.m_AppValues = nullptr; // not needed after write
}

} // end namespace adios
