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
                           const int transportIndex ) noexcept
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
        capsule.Write( variable.m_Values, GetTotalSize( localDimensions ), transportIndex );
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
void WriteHelper( CCapsule& capsule, CGroup& group, const std::string variableName, const T* values,
                  const int transportIndex, const bool debugMode )
{
    auto lf_DebugType = []( const bool debugMode, const std::string type, const std::set<std::string>& typeAliases )
    {
        if( debugMode == true )
        {
            if( typeAliases.count( type ) == 0 )
                throw std::invalid_argument( "ERROR: variable " + variableName + " is not of type " + type +
                                             " in call to Write\n" );
        }
    };

    const auto itVariable = group.m_Variables.find( variableName );

    if( debugMode == true )
    {
        if( itVariable == group.m_Variables.end() )
            throw std::invalid_argument( "ERROR: from Write function, variable " + variableName + " doesn't exist\n" );
    }

    const std::string type( itVariable->first );
    const unsigned int index = itVariable->second;
    group.m_SetVariables.insert( variableName ); //should be done before writing to buffer, in case there is a crash?

    //will need to add a lambda function later and put types in a set
    if( std::is_same<T,char>::value )
    {
        lf_DebugType( debugMode, type, SSupport::DatatypesAliases.at("char") );
        WriteHelperToCapsule( capsule, group, group.m_Char[index], values, transportIndex );
    }
    else if( std::is_same<T,unsigned char>::value )
    {
        lf_DebugType( debugMode, type, SSupport::DatatypesAliases.at("unsigned char") );
        WriteHelperToCapsule( capsule, group, group.m_UChar[index], values, transportIndex );
    }
    else if( std::is_same<T,short>::value )
    {
        lf_DebugType( debugMode, type, SSupport::DatatypesAliases.at("short") );
        WriteHelperToCapsule( capsule, group, group.m_Short[index], values, transportIndex );
    }
    else if( std::is_same<T,unsigned short>::value )
    {
        lf_DebugType( debugMode, type, SSupport::DatatypesAliases.at("unsigned short") );
        WriteHelperToCapsule( capsule, group, group.m_UShort[index], values, transportIndex );
    }
    else if( std::is_same<T,int>::value )
    {
        lf_DebugType( debugMode, type, SSupport::DatatypesAliases.at("int") );
        WriteHelperToCapsule( capsule, group, group.m_Int[index], values, transportIndex );
    }
    else if( std::is_same<T,unsigned int>::value )
    {
        lf_DebugType( debugMode, type, SSupport::DatatypesAliases.at("unsigned int") );
        WriteHelperToCapsule( capsule, group, group.m_UInt[index], values, transportIndex );
    }
    else if( std::is_same<T,long int>::value )
    {
        lf_DebugType( debugMode, type, SSupport::DatatypesAliases.at("long int") );
        WriteHelperToCapsule( capsule, group, group.m_LInt[index], values, transportIndex );
    }
    else if( std::is_same<T,unsigned long int>::value )
    {
        lf_DebugType( debugMode, type, SSupport::DatatypesAliases.at("unsigned long int") );
        WriteHelperToCapsule( capsule, group, group.m_ULInt[index], values, transportIndex );
    }
    else if( std::is_same<T,long long int>::value )
    {
        lf_DebugType( debugMode, type, SSupport::DatatypesAliases.at("long long int") );
        WriteHelperToCapsule( capsule, group, group.m_LLInt[index], values, transportIndex );
    }
    else if( std::is_same<T,unsigned long long int>::value )
    {
        lf_DebugType( debugMode, type, SSupport::DatatypesAliases.at("unsigned long long int") );
        WriteHelperToCapsule( capsule, group, group.m_ULLInt[index], values, transportIndex );
    }
    else if( std::is_same<T,float>::value )
    {
        lf_DebugType( debugMode, type, SSupport::DatatypesAliases.at("float") );
        WriteHelperToCapsule( capsule, group, group.m_Float[index], values, transportIndex );
    }
    else if( std::is_same<T,double>::value )
    {
        lf_DebugType( debugMode, type, SSupport::DatatypesAliases.at("double") );
        WriteHelperToCapsule( capsule, group, group.m_Double[index], values, transportIndex );
    }
    else if( std::is_same<T,long double>::value )
    {
        lf_DebugType( debugMode, type, SSupport::DatatypesAliases.at("long double") );
        WriteHelperToCapsule( capsule, group, group.m_LDouble[index], values, transportIndex );
    }
}




} //end namespace



#endif /* ADIOSTEMPLATES_H_ */
