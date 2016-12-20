/*
 * SIRIUS.h
 *
 *  Created on: Dec 16, 2016
 *      Author: wfg
 */

#ifndef SIRIUS_H_
#define SIRIUS_H_



namespace adios
{


class SIRIUS : public Engine
{

    SIRIUS( const std::string name, const std::string accessMode, const MPI_Comm mpiComm,
            const Method& method, const bool debugMode );

    ~SIRIUS( );




};


} //namespace



#endif /* SIRIUS_H_ */
