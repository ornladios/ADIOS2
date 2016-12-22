/*
 * Capsule.cpp
 *
 *  Created on: Nov 11, 2016
 *      Author: wfg
 */


#include "core/Capsule.h"


namespace adios
{


Capsule::Capsule( const std::string type, const std::string accessMode, const int rankMPI ):
    m_Type{ type },
    m_AccessMode{ accessMode },
    m_RankMPI{ rankMPI }
{ }


Capsule::~Capsule( )
{ }



} //end namespace
