/*
 * CFStream.h
 *
 *  Created on: Oct 18, 2016
 *      Author: wfg
 */

#ifndef FSTREAM_H_
#define FSTREAM_H_


#include <fstream>

#include "core/Transport.h"


namespace adios
{

/**
 * Class that defines a transport method using C++ file streams
 */
class FStream : public Transport
{

public:

    FStream( MPI_Comm mpiComm, const bool debugMode, const std::vector<std::string>& arguments );

    ~FStream( );

    void Open( const std::string streamName, const std::string accessMode );

    void SetBuffer( std::vector<char>& buffer );

    void Write( std::vector<char>& buffer );

    void Close( );

private:

    std::fstream m_FStream; ///< file stream corresponding to this transport

    void Init( const std::vector<std::string>& arguments );

};


} //end namespace



#endif /* FSTREAM_H_ */
