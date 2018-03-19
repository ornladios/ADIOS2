/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Sst.h
 *
 *  Created on: Aug 17, 2017
 *      Author: Greg Eisenhauer
 */

#ifndef ADIOS2_ENGINE_SST_SST_WRITER_TCC_
#define ADIOS2_ENGINE_SST_SST_WRITER_TCC_

#include "SstWriter.h"

#include "adios2/ADIOSMPI.h"
#include "adios2/helper/adiosFunctions.h" //GetType<T>

namespace adios2
{

template <class T>
void SstWriter::PutSyncCommon(Variable<T> &variable, const T *values)
{
    variable.SetData(values);

    // This part will go away, this is just to monitor variables per rank

    if (variable.m_Count.empty())
    {
        variable.m_Count = variable.m_Shape;
    }
    if (variable.m_Start.empty())
    {
        variable.m_Start.assign(variable.m_Count.size(), 0);
    }

    if (m_FFSmarshal)
    {
        SstFFSMarshal(m_Output, (void *)&variable, variable.m_Name.c_str(),
                      variable.m_Type.c_str(), variable.m_ElementSize,
                      variable.m_Shape.size(), variable.m_Shape.data(),
                      variable.m_Count.data(), variable.m_Start.data(), values);
    }
    else if (m_BPmarshal)
    {
		if (!m_BP3Serializer->m_MetadataSet.DataPGIsOpen)
		{
			m_BP3Serializer->PutProcessGroupIndex(m_IO.m_Name, m_IO.m_HostLanguage,
					{"SST"});
		}
		const size_t dataSize = variable.PayloadSize() +
			m_BP3Serializer->GetVariableBPIndexSize(
					variable.m_Name, variable.m_Count);
		format::BP3Base::ResizeResult resizeResult = m_BP3Serializer->ResizeBuffer(
				dataSize, "in call to variable " + variable.m_Name + " PutSync");
		m_BP3Serializer->PutVariableMetadata(variable);
		m_BP3Serializer->PutVariablePayload(variable);
    }
    else
    {
        // unknown marshaling method, shouldn't happen
    }
}

} // end namespace adios2

#endif /* ADIOS2_ENGINE_SST_SST_WRITER_H_ */
