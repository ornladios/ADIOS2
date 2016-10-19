/*
 * CSZIP.cpp
 *
 *  Created on: Oct 19, 2016
 *      Author: wfg
 */



#include "transform/CSZIP.h"



namespace adios
{

CSZIP::CSZIP( const unsigned int compressionLevel, CVariable& variable ):
    CTransform( "szip", compressionLevel, variable )
{ }

CSZIP::~CSZIP( )
{ }

void CSZIP::Compress( ) const
{ }

void CSZIP::Compress( ) const
{ }




} //end namespace
