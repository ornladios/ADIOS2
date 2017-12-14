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

    if (m_UseFormat == "json" || m_UseFormat == "JSON")
    {

        nlohmann::json jmsg;
        jmsg["doid"] = m_Name;
        jmsg["var"] = variable.m_Name;
        jmsg["dtype"] = GetType<T>();
        jmsg["putshape"] = variable.m_Count;
        jmsg["varshape"] = variable.m_Shape;
        jmsg["offset"] = variable.m_Start;
        jmsg["timestep"] = 0;
        jmsg["bytes"] =
            std::accumulate(variable.m_Shape.begin(), variable.m_Shape.end(),
                            sizeof(T), std::multiplies<size_t>());

        m_Man.WriteWAN(values, jmsg);
    }
    else if (m_UseFormat == "bp" || m_UseFormat == "BP")
    {
        PutSyncCommonBP(variable, values);
    }

    if (m_DoMonitor)
    {
        MPI_Barrier(m_MPIComm);
        std::cout << "I am hooked to the DataMan library\n";
        std::cout << "Variable " << variable.m_Name << "\n";
        std::cout << "putshape " << variable.m_Count.size() << "\n";
        std::cout << "varshape " << variable.m_Shape.size() << "\n";
        std::cout << "offset " << variable.m_Start.size() << "\n";

        int rank = 0, size = 1;
        MPI_Comm_size(m_MPIComm, &size);

        for (int i = 0; i < size; ++i)
        {
            if (i == rank)
            {
                std::cout << "Rank: " << i << "\n";
                std::cout << std::endl;
            }
        }
        MPI_Barrier(m_MPIComm);
    }
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

        m_Man.WriteWAN(buffer.data(), position);

        // set relative position to clear buffer
        m_BP3Serializer.ResetBuffer(m_BP3Serializer.m_Data);
        // new group index
        m_BP3Serializer.PutProcessGroupIndex(m_IO.m_HostLanguage, {"WAN_zmq"});
    }

    // WRITE INDEX to data buffer and metadata structure (in memory)//
    m_BP3Serializer.PutVariableMetadata(variable);
    m_BP3Serializer.PutVariablePayload(variable);
}

} // end namespace adios2

#endif /* ADIOS2_ENGINE_DATAMAN_DATAMAN_WRITER_H_ */
