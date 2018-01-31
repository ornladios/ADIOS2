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

namespace adios2
{

template <class T>
void DataManReader::GetSyncCommon(Variable<T> &variable, T *data)
{

    // delete any time steps older than the current step
	if(m_CurrentStep > m_OldestStep){
		for(int m = m_OldestStep; m < m_CurrentStep; ++m){
			auto k = m_VariableMap.find(m);
			if( k != m_VariableMap.end() ){
				m_MutexMap.lock();						    
				m_VariableMap.erase(k);
				m_MutexMap.unlock();											    
			}
		}
		m_OldestStep = m_CurrentStep;
	}

	bool looping = true;
	while(looping){
		looping = m_Blocking;
		auto i = m_VariableMap.find(m_CurrentStep);
		if( i != m_VariableMap.end() ){
			auto j = i->second.find(variable.m_Name);
			if( j != i->second.end() ){
				m_MutexMap.lock();
				m_MutexMap.unlock();
				std::memcpy(data, j->second->data.data(), j->second->data.size());
				return;
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
