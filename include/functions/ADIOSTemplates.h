/*
 * GroupTemplates.h
 *
 *  Created on: Nov 7, 2016
 *      Author: wfg
 */

#ifndef ADIOSTEMPLATES_H_
#define ADIOSTEMPLATES_H_

#include "core/CGroup.h"
#include "core/SVariable.h"
#include "core/CCapsule.h"


namespace adios
{

/**
 * Helper function called from ADIOS Write template function. Checks variable type, adds to group.m_SetVariables,
 * and calls corresponding Capsule virtual function.
 * @param variableName
 * @param values
 * @param group
 * @param capsule
 */
template< class T >
void WriteVariable( const std::string variableName, const T* values, CGroup& group, CCapsule& capsule )
{
    const bool debugMode( group.m_DebugMode );
    const std::string streamName( group.m_StreamName );
    const auto itVariable = group.m_Variables.find( variableName );

    if( debugMode == true )
    {
        if( itVariable == group.m_Variables.end() )
            throw std::invalid_argument( "ERROR: from Write function, variable " + variableName + " doesn't exist\n" );
    }

    const unsigned int index( itVariable->second.second ); //index is second in the pair Value of the m_Variables map
    group.m_SetVariables.insert( variableName );

    if( std::is_same<T,char>::value ) //maybe use type?
    {
        SVariable<char>& variable = group.m_Char[index];
        variable.m_Values = values;
        capsule.WriteVariableToBuffer( streamName, variable );
    }
    else if( std::is_same<T,unsigned char>::value )
    {
        SVariable<unsigned char>& variable = group.m_UChar[index];
        variable.m_Values = values;
        capsule.WriteVariableToBuffer( streamName, variable );
    }
    else if( std::is_same<T,short>::value )
    {
        SVariable<short>& variable = group.m_Short[index];
        variable.m_Values = values;
        capsule.WriteVariableToBuffer( streamName, variable );
    }
    else if( std::is_same<T,unsigned short>::value )
    {
        SVariable<unsigned short>& variable = group.m_UShort[index];
        variable.m_Values = values;
        capsule.WriteVariableToBuffer( streamName, variable );
    }
    else if( std::is_same<T,int>::value )
    {
        SVariable<int>& variable = group.m_Int[index];
        variable.m_Values = values;
        capsule.WriteVariableToBuffer( streamName, variable );
    }
    else if( std::is_same<T,unsigned int>::value )
    {
        SVariable<unsigned int>& variable = group.m_Int[index];
        variable.m_Values = values;
        capsule.WriteVariableToBuffer( streamName, variable );
    }
    else if( std::is_same<T,long int>::value )
    {
        SVariable<long int>& variable = group.m_LInt[index];
        variable.m_Values = values;
        capsule.WriteVariableToBuffer( streamName, variable );
    }
    else if( std::is_same<T,long long int>::value )
    {
        SVariable<long int>& variable = group.m_LLInt[index];
        variable.m_Values = values;
        capsule.WriteVariableToBuffer( streamName, variable );
    }
    else if( std::is_same<T,unsigned long long int>::value )
    {
        SVariable<unsigned long long int>& variable = group.m_ULLInt[index];
        variable.m_Values = values;
        capsule.WriteVariableToBuffer( streamName, variable );
    }
    else if( std::is_same<T,float>::value )
    {
        SVariable<float>& variable = group.m_Float[index];
        variable.m_Values = values;
        capsule.WriteVariableToBuffer( streamName, variable );
    }
    else if( std::is_same<T,double>::value )
    {
        SVariable<double>& variable = group.m_Double[index];
        variable.m_Values = values;
        capsule.WriteVariableToBuffer( streamName, variable );
    }
    else if( std::is_same<T,long double>::value )
    {
        SVariable<double>& variable = group.m_LDouble[index];
        variable.m_Values = values;
        capsule.WriteVariableToBuffer( streamName, variable );
    }
}



} //end namespace



#endif /* ADIOSTEMPLATES_H_ */
