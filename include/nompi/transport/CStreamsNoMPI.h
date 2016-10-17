/*
 * CStreamsNoMPI.h
 *
 *  Created on: Oct 17, 2016
 *      Author: wfg
 */

#ifndef CSTREAMSNOMPI_H_
#define CSTREAMSNOMPI_H_



#include "nompi/transport/CTransportNoMPI.h"


namespace adios
{



class CStreamsNoMPI : public CTransportNoMPI
{

public:

    CStreamsNoMPI( const unsigned int priority, const unsigned int iteration );

    ~CStreamsNoMPI( );

    void Write( const CVariable& variable );

};



} //end namespace






#endif /* CPOSIXNOMPI_H_ */




#endif /* CSTREAMSNOMPI_H_ */
