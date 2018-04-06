/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataManReader.tcc
 *
 *  Created on: Dec 8, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_ENGINE_DATAMAN_DATAMANREADER_TCC_
#define ADIOS2_ENGINE_DATAMAN_DATAMANREADER_TCC_

#include "DataManReader.h"
#include <iostream>
#include <limits>

namespace adios2
{

template <class T>
void DataManReader::GetSyncCommon(Variable<T> &variable, T *data)
{
    if(m_TransportMode == "subscribe"){
        m_MutexMap.lock();
        auto j = m_VariableMap[m_OldestStep].find(variable.m_Name);
        m_MutexMap.unlock();
        if( j != m_VariableMap[m_OldestStep].end() ){
            std::memcpy(data, j->second->data.data(), j->second->data.size());
            m_CurrentStep = m_OldestStep;
            return;
        }
    }
    else{
        // TODO: add timeout
        while(true){
            m_MutexMap.lock();
            auto i = m_VariableMap.find(m_CurrentStep);
            m_MutexMap.unlock();
            if( i != m_VariableMap.end() ){
                m_MutexMap.lock();
                auto j = i->second.find(variable.m_Name);
                m_MutexMap.unlock();
                if( j != i->second.end() ){
                    std::memcpy(data, j->second->data.data(), j->second->data.size());
                    return;
                }
            }
        }
    }
}

template <class T>
void DataManReader::GetDeferredCommon(Variable<T> &variable, T *data)
{
    GetSyncCommon(variable, data);
}


} // end namespace adios2

#endif /* ADIOS2_ENGINE_DATAMAN_DATAMANREADER_TCC_ */
