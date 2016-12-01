/*
 * CBZIP2.cpp
 *
 *  Created on: Oct 19, 2016
 *      Author: wfg
 */



#include "transform/CBZIP2.h"



namespace adios
{


CBZIP2::CBZIP2( ):
    CTransform( "bzip2" )
{ }


CBZIP2::~CBZIP2( )
{ }



void CBZIP2::Compress( const std::vector<char>& bufferIn, std::vector<char>& bufferOut )
{

}


void CBZIP2::Decompress( const std::vector<char>& bufferIn, std::vector<char>& bufferOut )
{

}



} //end namespace


