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

void EnginePy::WritePy( VariablePy<char>& variable, const pyArray& array )
{ m_Engine->Write( variable, PyArrayToPointer<char>( array ) ); }

void EnginePy::WritePy( VariablePy<unsigned char>& variable, const pyArray& array )
{ m_Engine->Write( variable, PyArrayToPointer<unsigned char>( array ) ); }

void EnginePy::WritePy( VariablePy<short>& variable, const pyArray& array )
{ m_Engine->Write( variable, PyArrayToPointer<short>( array ) ); }

void EnginePy::WritePy( VariablePy<unsigned short>& variable, const pyArray& array )
{ m_Engine->Write( variable, PyArrayToPointer<unsigned short>( array ) ); }

void EnginePy::WritePy( VariablePy<int>& variable, const pyArray& array )
{ m_Engine->Write( variable, PyArrayToPointer<int>( array ) ); }

void EnginePy::WritePy( VariablePy<unsigned int>& variable, const pyArray& array )
{ m_Engine->Write( variable, PyArrayToPointer<unsigned int>( array ) ); }

void EnginePy::WritePy( VariablePy<long int>& variable, const pyArray& array )
{ m_Engine->Write( variable, PyArrayToPointer<long int>( array ) ); }

void EnginePy::WritePy( VariablePy<unsigned long int>& variable, const pyArray& array )
{ m_Engine->Write( variable, PyArrayToPointer<unsigned long int>( array ) ); }

void EnginePy::WritePy( VariablePy<long long int>& variable, const pyArray& array )
{ m_Engine->Write( variable, PyArrayToPointer<long long int>( array ) ); }

void EnginePy::WritePy( VariablePy<unsigned long long int>& variable, const pyArray& array )
{ m_Engine->Write( variable, PyArrayToPointer<unsigned long long int>( array ) ); }

void EnginePy::WritePy( VariablePy<float>& variable, const pyArray& array )
{ m_Engine->Write( variable, PyArrayToPointer<float>( array ) ); }

void EnginePy::WritePy( VariablePy<double>& variable, const pyArray& array )
{ m_Engine->Write( variable, PyArrayToPointer<double>( array ) ); }

void EnginePy::WritePy( VariablePy<long double>& variable, const pyArray& array )
{ m_Engine->Write( variable, PyArrayToPointer<long double>( array ) ); }

void EnginePy::WritePy( VariablePy<std::complex<float>>& variable, const pyArray& array )
{ m_Engine->Write( variable, PyArrayToPointer<std::complex<float>>( array ) ); }

void EnginePy::WritePy( VariablePy<std::complex<double>>& variable, const pyArray& array )
{ m_Engine->Write( variable, PyArrayToPointer<std::complex<double>>( array ) ); }

void EnginePy::WritePy( VariablePy<std::complex<long double>>& variable, const pyArray& array )
{ m_Engine->Write( variable, PyArrayToPointer<std::complex<long double>>( array ) ); }

void EnginePy::GetType( ) const
{
	std::cout << "Engine type " << m_Engine->m_EngineType << "\n";
}

void EnginePy::Close( )
{
	m_Engine->Close( -1 );
}



} //end namespace
