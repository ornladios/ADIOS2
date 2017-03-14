/*
 * ADIOSPy.cpp
 *
 *  Created on: Mar 13, 2017
 *      Author: wfg
 */

#include <iostream>

#include "ADIOSPy.h"

namespace adios
{


ADIOSPy::ADIOSPy( MPI_Comm mpiComm, const bool debug ):
    ADIOS( mpiComm, debug )
{ }


ADIOSPy::~ADIOSPy( )
{ }


void ADIOSPy::HelloMPI( )
{
    std::cout << "Hello ADIOSPy from rank " << m_RankMPI << "/" << m_SizeMPI << "\n";
}


std::string ADIOSPy::DefineVariableFloat( const std::string name, const boost::python::list localDimensionsPy,
                                          const boost::python::list globalDimensionsPy, const boost::python::list globalOffsetsPy )
{
    return DefineVariablePy<float>( name, localDimensionsPy, globalDimensionsPy, globalOffsetsPy );
}


VariablePy<double>& ADIOSPy::DefineVariableDouble( const std::string name, const boost::python::list localDimensionsPy,
                                                 const boost::python::list globalDimensionsPy, const boost::python::list globalOffsetsPy )
{

	Variable<double>& var = DefineVariable<double>( name, ListToVector( localDimensionsPy ), ListToVector( globalDimensionsPy ), ListToVector( globalOffsetsPy ) );
	VariablePy<double>& varPy = *reinterpret_cast<VariablePy<double>*>( &var );
	return varPy;
}


void ADIOSPy::SetVariableLocalDimensions( const std::string name, const boost::python::list list )
{

    auto itVar = m_Variables.find( name );
    CheckVariableName( itVar, name, " in SetVariableLocalDimensions\n" );

    const std::string type = itVar->second.first;

    if( type == GetType<char>() )
        GetVariable<char>( name ).m_Dimensions = ListToVector( list );

    else if( type == GetType<unsigned char>() )
        GetVariable<unsigned char>( name ).m_Dimensions = ListToVector( list );

    else if( type == GetType<short>() )
        GetVariable<short>( name ).m_Dimensions = ListToVector( list );

    else if( type == GetType<unsigned short>() )
        GetVariable<unsigned short>( name ).m_Dimensions = ListToVector( list );

    else if( type == GetType<int>() )
        GetVariable<int>( name ).m_Dimensions = ListToVector( list );

    else if( type == GetType<unsigned int>() )
        GetVariable<unsigned int>( name ).m_Dimensions = ListToVector( list );

    else if( type == GetType<long int>() )
        GetVariable<long int>( name ).m_Dimensions = ListToVector( list );

    else if( type == GetType<unsigned long int>() )
        GetVariable<unsigned long int>( name ).m_Dimensions = ListToVector( list );

    else if( type == GetType<long long int>() )
        GetVariable<long long int>( name ).m_Dimensions = ListToVector( list );

    else if( type == GetType<unsigned long long int>() )
        GetVariable<unsigned long long int>( name ).m_Dimensions = ListToVector( list );

    else if( type == GetType<float>() )
        GetVariable<float>( name ).m_Dimensions = ListToVector( list );

    else if( type == GetType<double>() )
        GetVariable<double>( name ).m_Dimensions = ListToVector( list );

    else if( type == GetType<long double>() )
        GetVariable<long double>( name ).m_Dimensions = ListToVector( list );

    else if( type == GetType<std::complex<float>>() )
        GetVariable<std::complex<float>>( name ).m_Dimensions = ListToVector( list );

    else if( type == GetType<std::complex<double>>() )
        GetVariable<std::complex<double>>( name ).m_Dimensions = ListToVector( list );

    else if( type == GetType<std::complex<long double>>() )
        GetVariable<std::complex<long double>>( name ).m_Dimensions = ListToVector( list );
}


//boost::python::list ADIOSPy::GetVariableLocalDimensions( const std::string name )
std::vector<std::size_t> ADIOSPy::GetVariableLocalDimensions( const std::string name )
{
    auto itVar = m_Variables.find( name );
    CheckVariableName( itVar, name, " in SetVariableLocalDimensions\n" );

    const std::string type = itVar->second.first;

    std::vector<std::size_t> dims;

    if( type == GetType<char>() )
        dims = GetVariable<char>( name ).m_Dimensions;

    else if( type == GetType<unsigned char>() )
        dims = GetVariable<unsigned char>( name ).m_Dimensions;

    else if( type == GetType<short>() )
        dims = GetVariable<short>( name ).m_Dimensions;

    else if( type == GetType<unsigned short>() )
        dims = GetVariable<unsigned short>( name ).m_Dimensions;

    else if( type == GetType<int>() )
        dims = GetVariable<int>( name ).m_Dimensions;

    else if( type == GetType<unsigned int>() )
        dims = GetVariable<unsigned int>( name ).m_Dimensions;

    else if( type == GetType<long int>() )
        dims = GetVariable<long int>( name ).m_Dimensions;

    else if( type == GetType<unsigned long int>() )
        dims = GetVariable<unsigned long int>( name ).m_Dimensions;

    else if( type == GetType<long long int>() )
        dims = GetVariable<long long int>( name ).m_Dimensions;

    else if( type == GetType<unsigned long long int>() )
        dims = GetVariable<unsigned long long int>( name ).m_Dimensions;

    else if( type == GetType<float>() )
        dims = GetVariable<float>( name ).m_Dimensions;

    else if( type == GetType<double>() )
        dims = GetVariable<double>( name ).m_Dimensions;

    else if( type == GetType<long double>() )
        dims = GetVariable<long double>( name ).m_Dimensions;

    else if( type == GetType<std::complex<float>>() )
        dims = GetVariable<std::complex<float>>( name ).m_Dimensions;

    else if( type == GetType<std::complex<double>>() )
        dims = GetVariable<std::complex<double>>( name ).m_Dimensions;

    else if( type == GetType<std::complex<long double>>() )
        dims = GetVariable<std::complex<long double>>( name ).m_Dimensions;

    return dims;
    //return VectorToList( dims );
}




} //end namespace


