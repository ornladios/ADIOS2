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
void WriteVariableValues( CGroup& group, const std::string variableName, const T* values, CCapsule& capsule )
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

    if( std::is_same<T,char>::value ) //maybe use type?
    {
        auto& variable = group.m_Char[index];
        variable.m_Values = values;
        auto localDimensions = group.GetDimensions( variable.m_DimensionsCSV );

        if( variable.m_GlobalBoundsIndex > -1 ) //global variable
        {
            auto globalDimensions = group.GetDimensions( group.m_GlobalBounds[ variable.m_GlobalBoundsIndex ].first );
            auto globalOffsets = group.GetDimensions( group.m_GlobalBounds[ variable.m_GlobalBoundsIndex ].second );
            capsule.WriteDataToBuffer( variable.m_Values, sizeof(char), localDimensions, globalDimensions, globalOffsets );
        }
        else
        {
            capsule.WriteDataToBuffer( streamName, variable.m_Values, sizeof(char), localDimensions );
        }
    }
    else if( std::is_same<T,unsigned char>::value )
        group.m_UChar[index].m_Values = values;

    else if( std::is_same<T,short>::value )
        group.m_Short[index].m_Values = values;

    else if( std::is_same<T,unsigned short>::value )
        group.m_UShort[index].m_Values = values;

    else if( std::is_same<T,int>::value )
        group.m_Int[index].m_Values = values;

    else if( std::is_same<T,unsigned int>::value )
        group.m_UInt[index].m_Values = values;

    else if( std::is_same<T,long int>::value )
        group.m_LInt[index].m_Values = values;

    else if( std::is_same<T,unsigned long int>::value )
        group.m_ULInt[index].m_Values = values;

    else if( std::is_same<T,long long int>::value )
        group.m_LLInt[index].m_Values = values;

    else if( std::is_same<T,unsigned long long int>::value )
        group.m_ULLInt[index].m_Values = values;

    else if( std::is_same<T,float>::value )
        group.m_Float[index].m_Values = values;

    else if( std::is_same<T,double>::value )
        group.m_Double[index].m_Values = values;

    else if( std::is_same<T,long double>::value )
        group.m_LDouble[index].m_Values = values;

    group.m_SetVariables.insert( variableName );
}




} //end namespace



#endif /* ADIOSTEMPLATES_H_ */
