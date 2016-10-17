/*
 * CThreadNoMPI.h
 *
 *  Created on: Oct 17, 2016
 *      Author: wfg
 */

#ifndef CTHREADNOMPI_H_
#define CTHREADNOMPI_H_


#include "nompi/transport/CTransportNoMPI.h"


namespace adios
{


/**
 * Class that implementes asynchrounous I/O using C++ threads
 */
class CThreadNoMPI : public CTransportNoMPI
{

public:

    CThreadNoMPI( const unsigned int priority, const unsigned int iteration );

    ~CThreadNoMPI( );

    void Write( const CVariable& variable );

};



} //end namespace


#endif /* CTHREADNOMPI_H_ */
