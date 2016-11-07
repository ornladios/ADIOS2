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

template< class T >
void WriteVariable( const std::string variableName, const T* values, CGroup& group, CCapsule& capsule ) noexcept
{
    auto itVariable = group.m_Variables.find( variableName );
    const std::string type( itVariable->second.first );
    const unsigned int index( itVariable->second.second );
    group.m_SetVariables.insert( variableName );

    if( std::is_same<T,char> )
    {
        SVariable<char>& variable = group.m_Char[index];
        variable.m_Values = values;
        capsule.WriteVariableToBuffer( group, variable );
    }
    else if( std::is_same<T,unsigned char> )
    {
        SVariable<unsigned char>& variable = group.m_UChar[index];
        variable.m_Values = values;
        capsule.WriteVariableToBuffer( group, variable );
    }
    else if( std::is_same<T,short> )
    {
        SVariable<short>& variable = group.m_Short[index];
        variable.m_Values = values;
        capsule.WriteVariableToBuffer( group, variable );
    }
    else if( std::is_same<T,unsigned short> )
    {
        SVariable<unsigned short>& variable = group.m_UShort[index];
        variable.m_Values = values;
        capsule.WriteVariableToBuffer( group, variable );
    }
    else if( std::is_same<T,int> )
    {
        SVariable<int>& variable = group.m_Int[index];
        variable.m_Values = values;
        capsule.WriteVariableToBuffer( group, variable );
    }
    else if( std::is_same<T,unsigned int> )
    {
        SVariable<unsigned int>& variable = group.m_Int[index];
        variable.m_Values = values;
        capsule.WriteVariableToBuffer( group, variable );
    }
    else if( std::is_same<T,long int> )
    {
        SVariable<long int>& variable = group.m_LInt[index];
        variable.m_Values = values;
        capsule.WriteVariableToBuffer( group, variable );
    }
    else if( std::is_same<T,long long int> )
    {
        SVariable<long int>& variable = group.m_LLInt[index];
        variable.m_Values = values;
        capsule.WriteVariableToBuffer( group, variable );
    }
    else if( std::is_same<T,unsigned long long int> )
    {
        SVariable<unsigned long long int>& variable = group.m_ULLInt[index];
        variable.m_Values = values;
        capsule.WriteVariableToBuffer( group, variable );
    }
    else if( std::is_same<T,float> )
    {
        SVariable<float>& variable = group.m_Float[index];
        variable.m_Values = values;
        capsule.WriteVariableToBuffer( group, variable );
    }
    else if( std::is_same<T,double> )
    {
        SVariable<double>& variable = group.m_Double[index];
        variable.m_Values = values;
        capsule.WriteVariableToBuffer( group, variable );
    }
    else if( std::is_same<T,long double> )
    {
        SVariable<double>& variable = group.m_Double[index];
        variable.m_Values = values;
        capsule.WriteVariableToBuffer( group, variable );
    }
}



} //end namespace



#endif /* ADIOSTEMPLATES_H_ */
