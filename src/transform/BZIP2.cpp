/*
 * BZIP2.cpp
 *
 *  Created on: Oct 19, 2016
 *      Author: wfg
 */



#include "transform/BZIP2.h"



namespace adios
{


BZIP2::BZIP2( ):
    Transform( "bzip2" )
{ }


BZIP2::~BZIP2( )
{ }



void BZIP2::Compress( const std::vector<char>& bufferIn, std::vector<char>& bufferOut )
{

}


void BZIP2::Decompress( const std::vector<char>& bufferIn, std::vector<char>& bufferOut )
{

}



} //end namespace


