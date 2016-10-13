/*
 * CPOSIXNoMPI.h
 *
 *  Created on: Oct 6, 2016
 *      Author: wfg
 */

#ifndef CPOSIXNOMPI_H_
#define CPOSIXNOMPI_H_



#include "nompi/transport/CTransportNoMPI.h"


namespace adios
{



class CPOSIXNoMPI : public CTransportNoMPI
{

public:

    CPOSIXNoMPI( const std::string method, const unsigned int priority, const unsigned int iteration );

    ~CPOSIXNoMPI( );

    void Write( const CVariable& variable );

};



} //end namespace






#endif /* CPOSIXNOMPI_H_ */
