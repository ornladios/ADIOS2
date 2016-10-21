/*
 * CVariable.cpp
 *
 *  Created on: Oct 18, 2016
 *      Author: wfg
 */


#include "core/CVariable.h"


namespace adios
{


CVariable::CVariable( const bool isGlobal, const std::string type, const std::string dimensionsCSV, const std::string transform ):
    m_IsGlobal( isGlobal ),
    m_Type( type ),
    m_Transform( transform )
{
    if( dimensionsCSV == "1" )
    {
        m_Dimensions.push_back( "1" );
        return;
    }

    std::istringstream dimensionsCSVSS( dimensionsCSV );
    std::string dimension;
    while( std::getline( dimensionsCSVSS, dimension, ',' ) )
    {
        m_Dimensions.push_back( dimension );
    }
}

CVariable::~CVariable()
{ }


} //end namespace
