/*
 * CNetCDF4.h
 *
 *  Created on: Oct 17, 2016
 *      Author: wfg
 */

#ifndef CNETCDF4_H_
#define CNETCDF4_H_

#include <netcdf.h>

#include "core/CTransport.h"


namespace adios
{


class CNetCDF4 : public CTransport
{

public:

    CNetCDF4( const unsigned int priority, const unsigned int iteration, MPI_Comm mpiComm );

    ~CNetCDF4( );

};


} //end namespace


#endif /* CNETCDF4_H_ */
