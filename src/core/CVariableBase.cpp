/*
 * CVariable.cpp
 *
 *  Created on: Oct 18, 2016
 *      Author: wfg
 */


#include "core/CVariableBase.h"

namespace adios
{


CVariableBase::CVariableBase( const std::string type, const std::string dimensionsCSV, const std::string transform ):
    m_Type{ type },
    m_DimensionsCSV{ dimensionsCSV },
    m_Transform{ transform },
    m_IsGlobal{ false }
{ }

CVariableBase::CVariableBase( const std::string type, const std::string dimensionsCSV, const std::string transform,
                              const std::string globalDimensionsCSV, const std::string globalOffsetsCSV ):
    m_Type{ type },
    m_DimensionsCSV{ dimensionsCSV },
    m_Transform{ transform },
    m_GlobalDimensionsCSV{ globalDimensionsCSV },
    m_GlobalOffsetsCSV{ globalOffsetsCSV },
    m_IsGlobal{ true }
{ }


CVariableBase::~CVariableBase()
{ }



} //end namespace
