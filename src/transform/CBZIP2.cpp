/*
 * CBZIP2.cpp
 *
 *  Created on: Oct 19, 2016
 *      Author: wfg
 */



#include "transform/CBZIP2.h"



namespace adios
{


CBZIP2::CBZIP2( const unsigned int compressionLevel, CVariable& variable ):
    CTransform( "bzip2", compressionLevel, variable )
{ }

CBZIP2::~CBZIP2( )
{ }

void CBZIP2::Compress( ) const
{ }

void CBZIP2::Compress( ) const
{ }



} //end namespace


