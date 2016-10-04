/*
 * CPOSIXNoMPI.cpp
 *
 *  Created on: Oct 4, 2016
 *      Author: wfg
 */

#include "file_nompi/CPOSIXNoMPI.h"


namespace adios
{

CPOSIXNoMPI::CPOSIXNoMPI( const std::string fileName, const std::string fileType, const SMetadata& metadata ):
    CFileNoMPI( fileName, "POSIX_noMPI", metadata )
{ }


CPOSIXNoMPI::~CPOSIXNoMPI( )
{ }

void CPOSIXNoMPI::Open( const std::string fileName, const std::string groupName, const std::string accessMode )
{ }

unsigned long int CPOSIXNoMPI::GroupSize( const std::string fileName, const std::string groupName ) const
{
    return 0;
}

void CPOSIXNoMPI::Write( const std::string fileName, const std::string variableName )
{

}

void CPOSIXNoMPI::Close( )
{

}

} //end namespace


