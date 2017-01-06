/*
 * SingleBP.h
 *
 *  Created on: Dec 16, 2016
 *      Author: wfg
 */

#ifndef SINGLEBP_H_
#define SINGLEBP_H_

#include "core/Engine.h"


namespace adios
{


class SingleBP : public Engine
{

public:

    /**
     * Constructor for single BP capsule engine, writes in BP format into a single heap capsule
     * @param name unique name given to the engine
     * @param accessMode
     * @param mpiComm
     * @param method
     * @param debugMode
     */
    SingleBP( const std::string name, const std::string accessMode, MPI_Comm mpiComm,
              const Method& method, const bool debugMode = false, const unsigned int cores = 1 );

    ~SingleBP( );

    void Write( Group& group, const std::string variableName, const char* values );
    void Write( Group& group, const std::string variableName, const unsigned char* values );
    void Write( Group& group, const std::string variableName, const short* values );
    void Write( Group& group, const std::string variableName, const unsigned short* values );
    void Write( Group& group, const std::string variableName, const int* values );
    void Write( Group& group, const std::string variableName, const unsigned int* values );
    void Write( Group& group, const std::string variableName, const long int* values );
    void Write( Group& group, const std::string variableName, const unsigned long int* values );
    void Write( Group& group, const std::string variableName, const long long int* values );
    void Write( Group& group, const std::string variableName, const unsigned long long int* values );
    void Write( Group& group, const std::string variableName, const float* values );
    void Write( Group& group, const std::string variableName, const double* values );
    void Write( Group& group, const std::string variableName, const long double* values );


private:

    void Init( );
    void InitCapsules( );
    void InitTransports( );

};








} //end namespace


#endif /* SINGLEBP_H_ */
