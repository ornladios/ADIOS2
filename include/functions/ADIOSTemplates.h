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
void WriteHelperToCapsule( CCapsule& capsule, CGroup& group, SVariable<T>& variable, const T* values,
                           const unsigned int cores ) noexcept
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
        capsule.Write( variable.m_Values, GetTotalSize( localDimensions ), cores );
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
void WriteHelper( CCapsule& capsule, CGroup& group, const std::string variableName, const T* values, const bool debugMode,
                  const unsigned int cores )
{
    //variable
    const auto itVariable = group.m_Variables.find( variableName );

    if( debugMode == true )
    {
        if( itVariable == group.m_Variables.end() )
            throw std::invalid_argument( "ERROR: from Write function, variable " + variableName + " doesn't exist\n" );
    }
    const std::string type( itVariable->first );
    const unsigned int index = itVariable->second;
    group.m_SetVariables.insert( variableName ); //should be done before writing to buffer, in case there is a crash?

    if( std::is_same<T,char>::value )
    {
        if( debugMode == true )
        {
            if( type != "char" )
                throw std::invalid_argument( "ERROR: variable " + variableName + " is not char\n" );
        }
        WriteHelperToCapsule( capsule, group, group.m_Char[index], values, cores );
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
