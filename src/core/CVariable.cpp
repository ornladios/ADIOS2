/*
 * CVariable.cpp
 *
 *  Created on: Oct 18, 2016
 *      Author: wfg
 */


#include "core/CVariable.h"

namespace adios
{

CVariable::CVariable( const std::string type, const std::string dimensionsCSV, const std::string transform,
                              const std::string globalDimensionsCSV, const std::string globalOffsetsCSV ):
    m_Type{ type },
    m_DimensionsCSV{ dimensionsCSV },
    m_Transform{ transform },
    m_GlobalDimensionsCSV{ globalDimensionsCSV },
    m_GlobalOffsetsCSV{ globalOffsetsCSV }
{
    if( m_GlobalDimensionsCSV.empty() || m_GlobalOffsetsCSV.empty() )
        m_IsGlobal = false;
    else
        m_IsGlobal = true;
}


CVariable::~CVariable()
{ }


} //end namespace
