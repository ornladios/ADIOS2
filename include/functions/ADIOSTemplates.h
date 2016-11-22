/*
 * GroupTemplates.h
 *
 *  Created on: Nov 7, 2016
 *      Author: wfg
 */

#ifndef ADIOSTEMPLATES_H_
#define ADIOSTEMPLATES_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <string>
#include <stdexcept>
/// \endcond


#include "core/CGroup.h"
#include "core/SVariable.h"
#include "core/CCapsule.h"
#include "functions/ADIOSFunctions.h"


namespace adios
{

template<class T>
void WriteHelperToCapsule( CGroup& group, SVariable<T>& variable, const T* values, CCapsule& capsule, const unsigned int cores ) noexcept
{
    variable.m_Values = values;
    auto localDimensions = group.GetDimensions( variable.m_DimensionsCSV );

    if( variable.m_GlobalBoundsIndex > -1 ) //global variable
    {
        auto globalDimensions = group.GetDimensions( group.m_GlobalBounds[ variable.m_GlobalBoundsIndex ].first );
        auto globalOffsets = group.GetDimensions( group.m_GlobalBounds[ variable.m_GlobalBoundsIndex ].second );
        //capsule.Write( group.m_StreamName, variable.m_Values, sizeof(char), localDimensions, globalDimensions, globalOffsets );
    }
    else //write local variable
    {
        capsule.Write( group.m_StreamName, variable.m_Values, GetTotalSize( localDimensions ), cores );
    }
}



/**
 * Helper function called from ADIOS Write template function. Checks variable type, adds to group.m_SetVariables,
 * and calls corresponding Capsule virtual function.
 * @param variableName
 * @param values
 * @param group
 * @param capsule
 */
template<class T>
void WriteHelper( CGroup& group, const std::string variableName, const T* values, CCapsule& capsule, const unsigned int cores )
{
    const bool debugMode( group.m_DebugMode );
    const std::string streamName( group.m_StreamName );

    const auto itVariable = group.m_Variables.find( variableName );
    const std::string type( group.m_Variables.at( variableName ).first );
    unsigned int index = group.m_Variables.at( variableName ).second;

    if( debugMode == true )
    {
        if( itVariable == group.m_Variables.end() )
            throw std::invalid_argument( "ERROR: from Write function, variable " + variableName + " doesn't exist\n" );
    }

    group.m_SetVariables.insert( variableName );

    if( std::is_same<T,char>::value ) //maybe use type with debugMode?4
    {
        if( group.m_DebugMode == true )
        {
            if( type != "char" )
                throw std::invalid_argument( "ERROR: variable " + variableName + " is not char\n" );
        }
        WriteHelperToCapsule( group, group.m_Char[index], values, capsule, cores );
    }

//    else if( std::is_same<T,unsigned char>::value )
//        group.m_UChar[index].m_Values = values;
//
//    else if( std::is_same<T,short>::value )
//        group.m_Short[index].m_Values = values;
//
//    else if( std::is_same<T,unsigned short>::value )
//        group.m_UShort[index].m_Values = values;
//
//    else if( std::is_same<T,int>::value )
//        group.m_Int[index].m_Values = values;
//
//    else if( std::is_same<T,unsigned int>::value )
//        group.m_UInt[index].m_Values = values;
//
//    else if( std::is_same<T,long int>::value )
//        group.m_LInt[index].m_Values = values;
//
//    else if( std::is_same<T,unsigned long int>::value )
//        group.m_ULInt[index].m_Values = values;
//
//    else if( std::is_same<T,long long int>::value )
//        group.m_LLInt[index].m_Values = values;
//
//    else if( std::is_same<T,unsigned long long int>::value )
//        group.m_ULLInt[index].m_Values = values;
//
//    else if( std::is_same<T,float>::value )
//        group.m_Float[index].m_Values = values;
//
//    else if( std::is_same<T,double>::value )
//        group.m_Double[index].m_Values = values;
//
//    else if( std::is_same<T,long double>::value )
//        group.m_LDouble[index].m_Values = values;


}




} //end namespace



#endif /* ADIOSTEMPLATES_H_ */
