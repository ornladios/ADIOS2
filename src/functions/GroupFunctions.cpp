/*
 * GroupFunctions.cpp
 *
 *  Created on: Oct 27, 2016
 *      Author: wfg
 */
#include <complex>

#include "functions/GroupFunctions.h"
#include "core/CVariable.h"

//transports
#include "transport/CPOSIX.h"
#include "transport/CFStream.h"


namespace adios
{


void CreateVariableLanguage( const std::string name, const std::string type,
                             const std::string dimensionsCSV, const std::string transform,
                             const std::string globalDimensionsCSV, const std::string globalOffsetsCSV,
                             std::map<std::string, std::shared_ptr<CVariableBase> >& variables ) noexcept
{
    std::shared_ptr<CVariableBase> variable;
    //Common Primitive types to most languages
    if( type == "char" || type == "character" )
        variable = std::make_shared< CVariable<char> >( type, dimensionsCSV, transform, globalDimensionsCSV, globalOffsetsCSV );

    else if( type == "unsigned char" )
        variable = std::make_shared< CVariable<unsigned char> >( type, dimensionsCSV, transform, globalDimensionsCSV, globalOffsetsCSV );

    else if( type == "short" || type == "integer*2" )
        variables = std::make_shared< CVariable<short> >( type, dimensionsCSV, transform, globalDimensionsCSV, globalOffsetsCSV );

    else if( type == "unsigned short" )
        variables = std::make_shared< CVariable<unsigned short> >( type, dimensionsCSV, transform, globalDimensionsCSV, globalOffsetsCSV );

    else if( type == "int" || type == "integer" )
        variables = std::make_shared< CVariable<int> >( type, dimensionsCSV, transform, globalDimensionsCSV, globalOffsetsCSV );

    else if( type == "unsigned int" || type == "unsigned integer" )
        variable = std::make_shared< CVariable<unsigned int> >( type, dimensionsCSV, transform, globalDimensionsCSV, globalOffsetsCSV );

    else if( type == "long int" || type == "long" || type == "long integer" )
        variable = std::make_shared< CVariable<long int> >( type, dimensionsCSV, transform, globalDimensionsCSV, globalOffsetsCSV );

    else if( type == "long long int" || type == "long long" || type == "long long integer" )
        variable = std::make_shared< CVariable<long long int> >( type, dimensionsCSV, transform, globalDimensionsCSV, globalOffsetsCSV );

    else if( type == "float" || type == "real" || type == "real*4" )
        variable = std::make_shared< CVariable<float> >( type, dimensionsCSV, transform, globalDimensionsCSV, globalOffsetsCSV );

    else if( type == "double" || type == "double precision" || type == "real*8" )
        variable = std::make_shared< CVariable<double> >( type, dimensionsCSV, transform, globalDimensionsCSV, globalOffsetsCSV );

    else if( type == "long double" || type == "real*16" )
        variable = std::make_shared< CVariable<long double> >( type, dimensionsCSV, transform, globalDimensionsCSV, globalOffsetsCSV );

    //C++
    else if( type == "std::string" || type == "string" )
        variable = std::make_shared< CVariable<std::string> >( type, dimensionsCSV, transform, globalDimensionsCSV, globalOffsetsCSV );

    else if( type == "std::vector<char>" || type == "vector<char>" )
        variable = std::make_shared< CVariable< std::vector<char> > >( type, dimensionsCSV, transform, globalDimensionsCSV, globalOffsetsCSV );

    else if( type == "std::vector<int>" || type == "vector<int>" )
        variable = std::make_shared< CVariable< std::vector<int> > >( type, dimensionsCSV, transform, globalDimensionsCSV, globalOffsetsCSV );

    else if( type == "std::vector<unsigned int>" || type == "vector<unsigned int>" )
        variable = std::make_shared< CVariable< std::vector<unsigned int> > >( type, dimensionsCSV, transform, globalDimensionsCSV, globalOffsetsCSV );

    else if( type == "std::vector<long int>" || type == "std::vector<long>" ||
            type == "vector<long int>" || type == "vector<long>" )
        variable = std::make_shared< CVariable<std::vector<long int> > >( type, dimensionsCSV, transform, globalDimensionsCSV, globalOffsetsCSV );

    else if( type == "std::vector<long long int>" || type == "std::vector<long long>" ||
            type == "vector<long long int>" || type == "vector<long long>" )
        variable = std::make_shared< CVariable< std::vector<long long int> > >( type, dimensionsCSV, transform, globalDimensionsCSV, globalOffsetsCSV );

    else if( type == "std::vector<float>" || type == "vector<float>" )
        variable = std::make_shared< CVariable< std::vector<float> > >( type, dimensionsCSV, transform, globalDimensionsCSV, globalOffsetsCSV );

    else if( type == "std::vector<double>" || type == "vector<double>" )
        variable = std::make_shared< CVariable< std::vector<double> > >( type, dimensionsCSV, transform, globalDimensionsCSV, globalOffsetsCSV );

    else if( type == "std::vector<long double>" || type == "vector<long double>" )
        variable = std::make_shared< CVariable< std::vector<long double> > >( type, dimensionsCSV, transform, globalDimensionsCSV, globalOffsetsCSV );

    //STL Complex types
    else if( type == "std::complex<float>" || type == "complex<float>" )
        variable = std::make_shared< CVariable< std::complex<float> > >( type, dimensionsCSV, transform, globalDimensionsCSV, globalOffsetsCSV );

    else if( type == "std::complex<double>" || type == "complex<double>" )
        variable = std::make_shared< CVariable< std::complex<double> > >( type, dimensionsCSV, transform, globalDimensionsCSV, globalOffsetsCSV );

    else if( type == "std::complex<long double>" || type == "complex<long double>" )
        variable = std::make_shared< CVariable< std::complex<long double> > >( type, dimensionsCSV, transform, globalDimensionsCSV, globalOffsetsCSV );

    else if( type == "std::vector<std::complex<float>>" || type == "vector<complex<float>>" )
        variable = std::make_shared< CVariable< std::vector< std::complex<float> > > >( type, dimensionsCSV, transform, globalDimensionsCSV, globalOffsetsCSV );

    else if( type == "std::vector<std::complex<double>>" || type == "vector<complex<double>>" )
        variable = std::make_shared< CVariable< std::vector< std::complex<double> > > >( type, dimensionsCSV, transform, globalDimensionsCSV, globalOffsetsCSV );

    else if( type == "std::vector<std::complex<long double>>" || type == "vector<complex<long double>>" )
        variable = std::make_shared< CVariable< std::vector< std::complex<long double> > > >( type, dimensionsCSV, transform, globalDimensionsCSV, globalOffsetsCSV );

    variables[name] = variable;
}


void SetVariableValues( CVariableBase& variable, const void* values ) noexcept
{
    const std::string type( variable.m_Type );

    if( type == "char" || type == "character" )
        variable.Set<char>( values );

    else if( type == "unsigned char" )
        variable.Set<unsigned char>( values );

    else if( type == "short" || type == "integer*2" )
        variable.Set<short>( values );

    else if( type == "unsigned short" )
        variable.Set<unsigned short>( values );

    else if( type == "int" || type == "integer" )
        variable.Set<int>( values );

    else if( type == "unsigned int" || type == "unsigned integer" )
        variable.Set<unsigned int>( values );

    else if( type == "long int" || type == "long" || type == "long integer" )
        variable.Set<long int>( values );

    else if( type == "long long int" || type == "long long" || type == "long long integer" )
        variable.Set<long long int>( values );

    else if( type == "float" || type == "real" || type == "real*4" )
        variable.Set<float>( values );

    else if( type == "double" || type == "double precision" || type == "real*8" )
        variable.Set<double>( values );

    else if( type == "long double" || type == "real*16" )
        variable.Set<long double>( values );

    //C++
    else if( type == "std::string" || type == "string" )
        variable.Set<std::string>( values );

    else if( type == "std::vector<char>" || type == "vector<char>" )
        variable.Set< std::vector<char> >( values );

    else if( type == "std::vector<int>" || type == "vector<int>" )
        variable.Set< std::vector<int> >( values );

    else if( type == "std::vector<unsigned int>" || type == "vector<unsigned int>" )
        variable.Set< std::vector<unsigned int> >( values );

    else if( type == "std::vector<long int>" || type == "std::vector<long>" ||
            type == "vector<long int>" || type == "vector<long>" )
        variable.Set<std::vector<long int> >( values );

    else if( type == "std::vector<long long int>" || type == "std::vector<long long>" ||
            type == "vector<long long int>" || type == "vector<long long>" )
        variable.Set< std::vector<long long int> >( values );

    else if( type == "std::vector<float>" || type == "vector<float>" )
        variable.Set< std::vector<float> >( values );

    else if( type == "std::vector<double>" || type == "vector<double>" )
        variable.Set< std::vector<double> >( values );

    else if( type == "std::vector<long double>" || type == "vector<long double>" )
        variable.Set< std::vector<long double> >( values );

    //STL Complex types
    else if( type == "std::complex<float>" || type == "complex<float>" )
        variable.Set< std::complex<float> >( values );

    else if( type == "std::complex<double>" || type == "complex<double>" )
        variable.Set< std::complex<double> >( values );

    else if( type == "std::complex<long double>" || type == "complex<long double>" )
        variable.Set< std::complex<long double> >( values );

    else if( type == "std::vector<std::complex<float>>" || type == "vector<complex<float>>" )
        variable.Set< std::vector< std::complex<float> > >( values );

    else if( type == "std::vector<std::complex<double>>" || type == "vector<complex<double>>" )
        variable.Set< std::vector< std::complex<double> > >( values );

    else if( type == "std::vector<std::complex<long double>>" || type == "vector<complex<long double>>" )
        variable.Set< std::vector< std::complex<long double> > >( values );
}


void CreateTransport( const std::string method, const unsigned int priority, const unsigned int iteration,
                      const MPI_Comm mpiComm, const bool debugMode, std::shared_ptr<CTransport>& transport ) noexcept
{
    if( method == "POSIX" )
        transport = std::make_shared<CPOSIX>( priority, iteration, mpiComm, debugMode );

    else if( method == "FStream" )
        transport = std::make_shared<CFStream>( priority, iteration, mpiComm, debugMode );
}


} //end namespace


