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
	auto iter = m_VariableMap[0].find(variable.m_Name);
	if( iter != m_VariableMap[0].end() ){
		std::memcpy(data, iter->second->data.data(), iter->second->data.size());
		m_VariableMap[0].erase( iter );
	}
}

template <class T>
void DataManReader::GetDeferredCommon(Variable<T> &variable, T *data)
{
    GetSyncCommon(variable, data);
}

} // end namespace adios2

#endif /* ADIOS2_ENGINE_DATAMAN_DATAMANREADER_TCC_ */
