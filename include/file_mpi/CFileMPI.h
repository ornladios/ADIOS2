/*
 * CFileMPI.h
 *
 *  Created on: Oct 3, 2016
 *      Author: wfg
 */

#ifndef CFILEMPI_H_
#define CFILEMPI_H_

#include <mpi.h>

#include "CFile.h"


namespace adios
{


class CFileMPI : public CFile
{

public:

    MPI_Comm m_MPIComm; ///< passed from the application communicator through ADIOS m_MPIComm

    CFileMPI( const std::string fileName, const std::string fileType, MPI_Comm mpiComm ):
        CFile( fileName, fileType, true ),
        m_MPIComm( mpiComm )
    { }

    virtual ~CFileMPI( )
    { };

    virtual void Open( const std::string fileName, const std::string groupName, const std::string accessMode ) = 0;

    virtual unsigned long int GroupSize( const std::string fileName, const std::string groupName ) const = 0;

    virtual void Write( const std::string fileName, const std::string variableName ) = 0;

    virtual void Close( ) = 0;

};


} //end namespace



#endif /* CFILEMPI_H_ */
