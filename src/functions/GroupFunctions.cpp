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


void CreateVariableLanguage( const std::string hostLanguage, const std::string name, const bool isGlobal,
                             const std::string type, const std::string dimensionsCSV, const std::string transform,
                             std::map<std::string, std::shared_ptr<CVariableBase> >& variables ) noexcept
{
    if( hostLanguage == "C++" )
        CreateVariableCpp( name, isGlobal, type, dimensionsCSV, transform, variables );

    else if( hostLanguage == "C" )
        CreateVariableC( name, isGlobal, type, dimensionsCSV, transform, variables );

    else if( hostLanguage == "Fortran" )
        CreateVariableFortran( name, isGlobal, type, dimensionsCSV, transform, variables );
}


void CreateVariableCpp( const std::string name, const bool isGlobal,
                        const std::string type, const std::string dimensionsCSV, const std::string transform,
                        std::map< std::string, std::shared_ptr<CVariableBase> >& variables ) noexcept
{
    using PVar = std::shared_ptr<CVariableBase>; //might be modified in C++17 if std::make_shared is available, local scope
    //using emplace to create object in-place, can't use copy constructors with unique_ptr
    //Primitive types
    if( type == "char")
        variables.emplace( name, PVar( new CVariable<char>( isGlobal, type, dimensionsCSV, transform ) ) );
    else if( type == "unsigned char")
        variables.emplace( name, PVar( new CVariable<unsigned char>( isGlobal, type, dimensionsCSV, transform ) ) );

    else if( type == "int" || type == "integer" )
        variables.emplace( name, PVar( new CVariable<int>( isGlobal, type, dimensionsCSV, transform ) ) );

    else if( type == "unsigned int" )
        variables.emplace( name, PVar( new CVariable<unsigned int>( isGlobal, type, dimensionsCSV, transform ) ) );

    else if( type == "long int" || type == "long" )
        variables.emplace( name, PVar( new CVariable<long int>( isGlobal, type, dimensionsCSV, transform ) ) );

    else if( type == "long long int" || type == "long long" )
        variables.emplace( name, PVar( new CVariable<long long int>( isGlobal, type, dimensionsCSV, transform ) ) );

    else if( type == "float" )
        variables.emplace( name, PVar( new CVariable<float>( isGlobal, type, dimensionsCSV, transform ) ) );

    else if( type == "double")
        variables.emplace( name, PVar( new CVariable<double>( isGlobal, type, dimensionsCSV, transform ) ) );

    else if( type == "long double")
        variables.emplace( name, PVar( new CVariable<long double>( isGlobal, type, dimensionsCSV, transform ) ) );

    //STL types
    else if( type == "std::vector<char>" || type == "vector<char>" )
        variables.emplace( name, PVar( new CVariable< std::vector<char> >( isGlobal, type, dimensionsCSV, transform ) ) );

    else if( type == "std::vector<int>" || type == "vector<int>" )
        variables.emplace( name, PVar( new CVariable< std::vector<int> >( isGlobal, type, dimensionsCSV, transform ) ) );

    else if( type == "std::vector<unsigned int>" || type == "vector<unsigned int>" )
        variables.emplace( name, PVar( new CVariable< std::vector<unsigned int> >( isGlobal, type, dimensionsCSV, transform ) ) );

    else if( type == "std::vector<long int>" || type == "std::vector<long>" ||
             type == "vector<long int>" || type == "vector<long>" )
        variables.emplace( name, PVar( new CVariable<std::vector<long int> >( isGlobal, type, dimensionsCSV, transform ) ) );

    else if( type == "std::vector<long long int>" || type == "std::vector<long long>" ||
             type == "vector<long long int>" || type == "vector<long long>" )
        variables.emplace( name, PVar( new CVariable< std::vector<long long int> >( isGlobal, type, dimensionsCSV, transform ) ) );

    else if( type == "std::vector<float>" || type == "vector<float>" )
        variables.emplace( name, PVar( new CVariable< std::vector<float> >( isGlobal, type, dimensionsCSV, transform ) ) );

    else if( type == "std::vector<double>" || type == "vector<double>" )
        variables.emplace( name, PVar( new CVariable< std::vector<double> >( isGlobal, type, dimensionsCSV, transform ) ) );

    else if( type == "std::vector<long double>" || type == "vector<long double>" )
        variables.emplace( name, PVar( new CVariable< std::vector<long double> >( isGlobal, type, dimensionsCSV, transform ) ) );

    //STL Complex types
    else if( type == "std::complex<float>" || type == "complex<float>" )
        variables.emplace( name, PVar( new CVariable< std::complex<float> >( isGlobal, type, dimensionsCSV, transform ) ) );

    else if( type == "std::complex<double>" || type == "complex<double>" )
        variables.emplace( name, PVar( new CVariable< std::complex<double> >( isGlobal, type, dimensionsCSV, transform ) ) );

    else if( type == "std::complex<long double>" || type == "complex<long double>" )
        variables.emplace( name, PVar( new CVariable< std::complex<long double> >( isGlobal, type, dimensionsCSV, transform ) ) );

    else if( type == "std::vector<std::complex<float>>" || type == "vector<complex<float>>" )
        variables.emplace( name, PVar( new CVariable< std::vector< std::complex<float> > >( isGlobal, type, dimensionsCSV, transform ) ) );

    else if( type == "std::vector<std::complex<double>>" || type == "vector<complex<double>>" )
        variables.emplace( name, PVar( new CVariable< std::vector< std::complex<double> > >( isGlobal, type, dimensionsCSV, transform ) ) );

    else if( type == "std::vector<std::complex<long double>>" || type == "vector<complex<long double>>" )
        variables.emplace( name, PVar( new CVariable< std::vector< std::complex<long double> > >( isGlobal, type, dimensionsCSV, transform ) ) );
}


void CreateVariableC( const std::string name, const bool isGlobal,
                      const std::string type, const std::string dimensionsCSV, const std::string transform,
                      std::map< std::string, std::shared_ptr<CVariableBase> >& variables ) noexcept
{

}


void CreateVariableFortran( const std::string name, const bool isGlobal,
                            const std::string type, const std::string dimensionsCSV, const std::string transform,
                            std::map< std::string, std::shared_ptr<CVariableBase> >& variables ) noexcept
{

}


void SetVariableValues( CVariableBase& variable, const void* values ) noexcept
{
    const std::string type = variable.m_Type;

    //Set variable values, add more support
    if( type == "double" )
        variable.Set<double>( values );

    else if( type == "int" || type == "integer" )
        variable.Set<int>( values );

    else if( type == "std::vector<int>" || type == "vector<int>"  )
        variable.Set<std::vector<int>>( values );

    else if( type == "std::vector<double>" || type == "vector<double>"  )
            variable.Set<std::vector<double>>( values );

}


void CreateTransport( const std::string method, const unsigned int priority, const unsigned int iteration,
                      const MPI_Comm mpiComm, std::shared_ptr<CTransport>& transport ) noexcept
{
    using PCT = std::shared_ptr<CTransport>; //might be modified in C++14 if std::make_unique is available, local scope

    if( method == "POSIX" )
        transport = PCT( new CPOSIX( priority, iteration, mpiComm ) );

    else if( method == "FStream" )
        transport = PCT( new CFStream( priority, iteration, mpiComm ) );
}


} //end namespace


