/*
 * File.h
 *
 *  Created on: Jan 6, 2017
 *      Author: wfg
 */

#ifndef FP_H_
#define FP_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <stdio.h> // FILE*
/// \endcond

#include "core/Transport.h"


namespace adios
{
namespace transport
{

/**
 * Class that defines a transport method using C file pointer (FP) to streams FILE*
 */
class FP : public Transport
{

public:

    FP( MPI_Comm mpiComm, const bool debugMode );

    ~FP( );

    void Open( const std::string name, const std::string accessMode );

    void SetBuffer( char* buffer, std::size_t size );

    void Write( const char* buffer, std::size_t size );

    void Flush( );

    void Close( );


private:

    FILE* m_File = NULL; ///< C file pointer

};


} //end namespace transport
} //end namespace




#endif /* FP_H_ */
