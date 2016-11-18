/*
 * CPOSIXMPI.h
 *
 *  Created on: Oct 6, 2016
 *      Author: wfg
 */

#ifndef CPOSIX_H_
#define CPOSIX_H_

#include <stdio.h> //FILE*

#include "core/CTransport.h"


namespace adios
{


class CPOSIX : public CTransport
{

public:

    CPOSIX( MPI_Comm mpiComm, const bool debugMode  );

    ~CPOSIX( );

    void Open( const std::string streamName, const std::string accessMode );

    void SetBuffer( std::vector<char>& buffer );

    void Close( );


private:

    FILE* m_File;

};


} //end namespace


#endif /* CPOSIX_H_ */
