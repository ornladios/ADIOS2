/*
 * CFileNoMPI.h
 *
 *  Created on: Oct 3, 2016
 *      Author: wfg
 */

#ifndef CFILENOMPI_H_
#define CFILENOMPI_H_


#include "CFile.h"


namespace adios
{

class CFileNoMPI : public CFile
{

public:

    CFileNoMPI( const std::string fileName, const std::string fileType, const SMetadata& metadata ):
        CFile( fileName, fileName, metadata, false )
    { };

    virtual ~CFileNoMPI( ){ };

    virtual void Open( const std::string fileName, const std::string groupName, const std::string accessMode ) = 0;

    virtual unsigned long int GroupSize( const std::string fileName, const std::string groupName ) const = 0;

    virtual void Write( const std::string fileName, const std::string variableName ) = 0;

    virtual void Close( ) = 0;

};


} //end namespace



#endif /* CFILENOMPI_H_ */
