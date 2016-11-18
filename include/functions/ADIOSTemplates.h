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

/**
 * Helper function called from ADIOS Write template function. Checks variable type, adds to group.m_SetVariables,
 * and calls corresponding Capsule virtual function.
 * @param variableName
 * @param values
 * @param group
 * @param capsule
 */
template<class T>
void WriteVariableValues( CGroup& group, const std::string variableName, const T* values, CCapsule& capsule, const unsigned int cores )
{
    const bool debugMode( group.m_DebugMode );
    const std::string streamName( group.m_StreamName );
    const auto itVariable = group.m_Variables.find( variableName );

    if( debugMode == true )
    {
        if( itVariable == group.m_Variables.end() )
            throw std::invalid_argument( "ERROR: from Write function, variable " + variableName + " doesn't exist\n" );
    }

    if( std::is_same<T,char>::value ) //maybe use type with debugMode?
        WriteChar( group, variableName, values, capsule, cores );

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

    group.m_SetVariables.insert( variableName );
}




} //end namespace



#endif /* ADIOSTEMPLATES_H_ */
