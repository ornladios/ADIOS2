/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataMan.h
 *
 *  Created on: Jan 10, 2017
 *      Author: wfg
 */

#ifndef ADIOS2_ENGINE_DATAMAN_DATAMAN_WRITER_TCC_
#define ADIOS2_ENGINE_DATAMAN_DATAMAN_WRITER_TCC_

#include "DataManWriter.h"

#include "adios2/ADIOSMPI.h"
#include "adios2/helper/adiosFunctions.h" //GetType<T>

#include <iostream>

namespace adios2
{

template <class T>
void DataManWriter::PutSyncCommon(Variable<T> &variable, const T *values)
{

    variable.SetData(values);

    if (variable.m_Shape.empty())
    {
        variable.m_Shape = variable.m_Count;
    }
    if (variable.m_Count.empty())
    {
        variable.m_Count = variable.m_Shape;
    }
    if (variable.m_Start.empty())
    {
        variable.m_Start.assign(variable.m_Count.size(), 0);
    }

    if (m_UseFormat == "bp")
    {
        PutSyncCommonBP(variable, values);
    }

    else if (m_UseFormat == "json")
    {
        PutSyncCommonJson(variable, values);
    }

}

template <class T>
void DataManWriter::PutSyncCommonJson(Variable<T> &variable, const T *values)
{
	nlohmann::json metaj;
	
	metaj["S"] = variable.m_Shape;
	metaj["C"] = variable.m_Count;
	metaj["O"] = variable.m_Start;
	metaj["T"] = m_CurrentStep;
	metaj["N"] = variable.m_Name;
	metaj["Y"] = variable.m_Type;
	metaj["I"] = variable.PayloadSize();

	std::string metastr = metaj.dump();
	size_t flagsize = sizeof(size_t);
	size_t metasize = metastr.size();
	size_t datasize = variable.PayloadSize();
	size_t totalsize = flagsize + metasize + datasize;

	std::shared_ptr<std::vector<char>> buffer = std::make_shared<std::vector<char>>(totalsize);
	std::memcpy(buffer->data(), &metasize, flagsize);
	std::memcpy(buffer->data() + flagsize, metastr.c_str(), metasize);
	std::memcpy(buffer->data() + flagsize + metasize, values, datasize);

	m_Man.WriteWAN(buffer);
}


template <class T>
void DataManWriter::PutSyncCommonBP(Variable<T> &variable, const T *values)
{
    // add bp serialization
    variable.SetData(values);

    // if first timestep Write create a new pg index
    if (!m_BP3Serializer.m_MetadataSet.DataPGIsOpen)
    {
        m_BP3Serializer.PutProcessGroupIndex(m_IO.m_HostLanguage, {"WAN_Zmq"});
    }

    const size_t dataSize = variable.PayloadSize() +
                            m_BP3Serializer.GetVariableBPIndexSize(
                                variable.m_Name, variable.m_Count);
    format::BP3Base::ResizeResult resizeResult = m_BP3Serializer.ResizeBuffer(
        dataSize, "in call to variable " + variable.m_Name + " PutSync");

    if (resizeResult == format::BP3Base::ResizeResult::Flush)
    {
        // Close buffer here?
        m_BP3Serializer.CloseStream(m_IO);
        auto &buffer = m_BP3Serializer.m_Data.m_Buffer;
        auto &position = m_BP3Serializer.m_Data.m_Position;

        m_Man.WriteWAN(buffer);

        // set relative position to clear buffer
        m_BP3Serializer.ResetBuffer(m_BP3Serializer.m_Data);
        // new group index
        m_BP3Serializer.PutProcessGroupIndex(m_IO.m_HostLanguage, {"WAN_zmq"});
    }

    // WRITE INDEX to data buffer and metadata structure (in memory)//
    m_BP3Serializer.PutVariableMetadata(variable);
    m_BP3Serializer.PutVariablePayload(variable);
}

template <class T>
void DataManWriter::PutDeferredCommon(Variable<T> &variable, const T *values)
{
    PutSyncCommon(variable, values);

}

} // end namespace adios2

#endif /* ADIOS2_ENGINE_DATAMAN_DATAMAN_WRITER_H_ */
