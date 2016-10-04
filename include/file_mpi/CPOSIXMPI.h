/*
 * CPOSIXMPI.h
 *
 *  Created on: Oct 4, 2016
 *      Author: wfg
 */

#ifndef CPOSIXMPI_H_
#define CPOSIXMPI_H_

#include "file_mpi/CFileMPI.h"

namespace adios
{

class CPOSIXMPI : public CFileMPI
{

public:

    CPOSIXMPI( const std::string fileName, const SMetadata& metadata, const MPI_Comm& mpiComm );

    ~CPOSIXMPI( );

    void Open( const std::string fileName, const std::string groupName, const std::string accessMode );

    unsigned long int GroupSize( const std::string fileName, const std::string groupName ) const;

    void Write( const std::string fileName, const std::string variableName );

    void Close( );
};


} //end namespace




#endif /* CPOSIXMPI_H_ */
