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

EnginePy::EnginePy( ADIOSPy& adiosPy ):
    m_ADIOSPy{ adiosPy }
{ }


EnginePy::~EnginePy( )
{ }


void EnginePy::WritePy( VariablePy& variable, const pyArray& array )
{
    const dtype arrayDType = DType( array );

    if( variable.m_IsVariableDefined == false ) //here define variable
    {
             if( arrayDType == GetDType<char>() ) DefineVariableInADIOS<char>( variable );
        else if( arrayDType == GetDType<unsigned char>() ) DefineVariableInADIOS<unsigned char>( variable );
        else if( arrayDType == GetDType<short>() ) DefineVariableInADIOS<short>( variable );
        else if( arrayDType == GetDType<unsigned short>() ) DefineVariableInADIOS<unsigned short>( variable );
        else if( arrayDType == GetDType<int>() ) DefineVariableInADIOS<int>( variable );
        else if( arrayDType == GetDType<unsigned int>() ) DefineVariableInADIOS<unsigned int>( variable );
        else if( arrayDType == GetDType<long int>() ) DefineVariableInADIOS<long int>( variable );
        else if( arrayDType == GetDType<unsigned long int>() ) DefineVariableInADIOS<unsigned long int>( variable );
        else if( arrayDType == GetDType<long long int>() ) DefineVariableInADIOS<long long int>( variable );
        else if( arrayDType == GetDType<unsigned long long int>() ) DefineVariableInADIOS<unsigned long long int>( variable );
        else if( arrayDType == GetDType<float>() ) DefineVariableInADIOS<float>( variable );
        else if( arrayDType == GetDType<double>() ) DefineVariableInADIOS<double>( variable );
        else if( arrayDType == GetDType<long double>() ) DefineVariableInADIOS<long double>( variable );
        else if( arrayDType == GetDType<std::complex<float>>() ) DefineVariableInADIOS<std::complex<float>>( variable );
        else if( arrayDType == GetDType<std::complex<double>>() ) DefineVariableInADIOS<std::complex<double>>( variable );
        else if( arrayDType == GetDType<std::complex<long double>>() ) DefineVariableInADIOS<std::complex<long double>>( variable );
    }

         if( arrayDType == GetDType<char>() ) WriteVariableInADIOS<char>( variable, array );
    else if( arrayDType == GetDType<unsigned char>() ) WriteVariableInADIOS<unsigned char>( variable, array );
    else if( arrayDType == GetDType<short>() ) WriteVariableInADIOS<short>( variable, array );
    else if( arrayDType == GetDType<unsigned short>() ) WriteVariableInADIOS<unsigned short>( variable, array );
    else if( arrayDType == GetDType<int>() ) WriteVariableInADIOS<int>( variable, array );
    else if( arrayDType == GetDType<unsigned int>() ) WriteVariableInADIOS<unsigned int>( variable, array );
    else if( arrayDType == GetDType<long int>() ) WriteVariableInADIOS<long int>( variable, array );
    else if( arrayDType == GetDType<unsigned long int>() ) WriteVariableInADIOS<unsigned long int>( variable, array );
    else if( arrayDType == GetDType<long long int>() ) WriteVariableInADIOS<long long int>( variable, array );
    else if( arrayDType == GetDType<unsigned long long int>() ) WriteVariableInADIOS<unsigned long long int>( variable, array );
    else if( arrayDType == GetDType<float>() ) WriteVariableInADIOS<float>( variable, array );
    else if( arrayDType == GetDType<double>() ) WriteVariableInADIOS<double>( variable, array );
    else if( arrayDType == GetDType<long double>() ) WriteVariableInADIOS<long double>( variable, array );
    else if( arrayDType == GetDType<std::complex<float>>() ) WriteVariableInADIOS<std::complex<float>>( variable, array );
    else if( arrayDType == GetDType<std::complex<double>>() ) WriteVariableInADIOS<std::complex<double>>( variable, array );
    else if( arrayDType == GetDType<std::complex<long double>>() ) WriteVariableInADIOS<std::complex<long double>>( variable, array );
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
