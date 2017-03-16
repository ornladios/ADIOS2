/*
 * EnginePy.cpp
 *
 *  Created on: Mar 15, 2017
 *      Author: wgodoy
 */



#include "EnginePy.h"

#include "adiosPyFunctions.h"

namespace adios
{


void EnginePy::WritePy( VariablePy<double>& variable, const np::ndarray& array )
{
    const double* values = reinterpret_cast<const double*>( array.get_data() );
    m_Engine->Write( variable, values );
}

void EnginePy::WritePy( VariablePy<float>& variable, const np::ndarray& array )
{
	const float* values = reinterpret_cast<const float*>( array.get_data() );
	m_Engine->Write( variable, values );
}

void EnginePy::GetType( ) const
{
	std::cout << "Engine type " << m_Engine->m_EngineType << "\n";
}

void EnginePy::Close( )
{
	m_Engine->Close( -1 );
}



} //end namespace
