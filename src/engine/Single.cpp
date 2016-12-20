/*
 * Single.cpp
 *
 *  Created on: Dec 19, 2016
 *      Author: wfg
 */


#include "engine/Single.h"

namespace adios
{

Single::Single( const std::string streamName, const std::string accessMode, const MPI_Comm mpiComm, const Method& method,
                const bool debugMode ):
    Engine( "Single Buffer Engine", streamName, accessMode, mpiComm, method, debugMode )
{
    if( m_DebugMode == true )
    {
        if( m_Method->Capsules.size() > 1 )
            throw std::invalid_argument( "ERROR: single buffer engine only allows one buffer, from Single constructor in Open.\n" );
    }

    SetTransports( );
}


Single::~Single( )
{ }



}

