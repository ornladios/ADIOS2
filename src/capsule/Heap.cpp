/*
 * Heap.cpp
 *
 *  Created on: Dec 22, 2016
 *      Author: wfg
 */


#include "capsule/Heap.h"



namespace adios
{

Heap::Heap( const std::string accessMode, const int rankMPI, const size_t maxDataSize, const size_t maxMetadataSize ):
    Capsule( "Heap", accessMode, rankMPI ),
    m_MaxDataSize{ maxDataSize },
    m_MaxMetadataSize{ maxMetadataSize }
{

}


Heap::~Heap( )
{ }




}  //end namespace
