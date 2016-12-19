/*
 * Single.h
 *
 *  Created on: Dec 16, 2016
 *      Author: wfg
 */

#ifndef SINGLE_H_
#define SINGLE_H_

#include "core/Engine.h"


namespace adios
{


class Single : public Engine
{

public:

    /**
     * Constructor for single capsule engine
     * @param streamName
     * @param accessMode
     * @param mpiComm
     * @param method
     * @param debugMode
     */
    Single( const std::string streamName, const std::string accessMode, const MPI_Comm mpiComm, const Method& method,
            const bool debugMode = false );


    ~Single( );

};








} //end namespace


#endif /* SINGLE_H_ */
