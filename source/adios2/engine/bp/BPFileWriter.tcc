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

#include <cmath>

namespace adios
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

    format::BP1Base::ResizeResult resizeResult =
        m_BP1Writer.ResizeBuffer(variable);

    // WRITE INDEX to data buffer and metadata structure (in memory)//
    m_BP1Writer.WriteVariableMetadata(variable);

    if (resizeResult == format::BP1Base::ResizeResult::FLUSH)
    {
        auto &heapBuffer = m_BP1Writer.m_HeapBuffer;

        // first batch fills current buffer and sends to transports
        const size_t firstBatchSize = heapBuffer.GetAvailableDataSize();
        CopyToBuffer(heapBuffer.m_Data, heapBuffer.m_DataPosition, values,
                     firstBatchSize / sizeof(T));
        m_TransportsManager.WriteFiles(heapBuffer.GetData(),
                                       heapBuffer.m_DataPosition);

        // start writing missing size in batches directly to transport
        const size_t missingSize = variable.PayLoadSize() - firstBatchSize;
        const size_t bufferSize = heapBuffer.GetDataSize();
        const size_t batches = missingSize / bufferSize;
        const size_t lastSize = missingSize % batches;

        // flush to transports in uniform batches
        for (size_t batch = 0; batch < batches; ++batch)
        {
            const size_t start = batch * bufferSize / sizeof(T);
            const char *valuesPtr =
                reinterpret_cast<const char *>(&values[start]);

            if (batch == batches - 1) // lastSize
            {
                m_TransportsManager.WriteFiles(valuesPtr, lastSize);
            }
            else
            {
                m_TransportsManager.WriteFiles(valuesPtr, bufferSize);
            }
        }

        // update absolute position
        heapBuffer.m_DataAbsolutePosition += variable.PayLoadSize();
        // set relative position to zero
        heapBuffer.m_DataPosition = 0;
        // reset buffer to zero values
        heapBuffer.m_Data.assign(heapBuffer.GetDataSize(), '\0');
    }
    else // Write data to buffer
    {
        m_BP1Writer.WriteVariablePayload(variable);
    }
    variable.m_AppValues = nullptr; // not needed after write
}

} // end namespace adios
