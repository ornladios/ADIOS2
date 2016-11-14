/*
 * CFStream.h
 *
 *  Created on: Oct 18, 2016
 *      Author: wfg
 */

#ifndef CFSTREAM_H_
#define CFSTREAM_H_

#include <fstream>

#include "core/CTransport.h"


namespace adios
{

/**
 * Class that defines a transport method using C++ file streams
 */
class CFStream : public CTransport
{

public:

    CFStream( MPI_Comm mpiComm, const bool debugMode );

    ~CFStream( );

    void Open( const std::string streamName, const std::string accessMode );

    void SetBuffer( std::vector<char>& buffer );

    void Close( );

private:

    std::fstream m_FStream; ///< file stream corresponding to this transport

};


} //end namespace



#endif /* CFSTREAM_H_ */
