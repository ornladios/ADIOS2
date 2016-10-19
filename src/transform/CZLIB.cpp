/*
 * CZLIB.cpp
 *
 *  Created on: Oct 19, 2016
 *      Author: wfg
 */



#include "transform/CZLIB.h"



namespace adios
{

CZLIB::CZLIB( const unsigned int compressionLevel, CVariable& variable ):
    CTransform( "szip", compressionLevel, variable )
{ }

CZLIB::~CZLIB( )
{ }

void CZLIB::Compress( ) const
{ }

void CZLIB::Decompress( ) const
{ }




} //end namespace
