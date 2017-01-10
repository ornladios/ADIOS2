/*
 * DataMan.h
 *
 *  Created on: Jan 10, 2017
 *      Author: wfg
 */

#ifndef DATAMAN_H_
#define DATAMAN_H_


#include "core/Engine.h"


namespace adios
{
namespace engine
{


class DataMan : public Engine
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
    DataMan( const std::string name, const std::string accessMode, MPI_Comm mpiComm,
              const Method& method, const bool debugMode = false, const unsigned int cores = 1 );

    ~DataMan( );

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

    void Write( const std::string variableName, const char* values );
    void Write( const std::string variableName, const unsigned char* values );
    void Write( const std::string variableName, const short* values );
    void Write( const std::string variableName, const unsigned short* values );
    void Write( const std::string variableName, const int* values );
    void Write( const std::string variableName, const unsigned int* values );
    void Write( const std::string variableName, const long int* values );
    void Write( const std::string variableName, const unsigned long int* values );
    void Write( const std::string variableName, const long long int* values );
    void Write( const std::string variableName, const unsigned long long int* values );
    void Write( const std::string variableName, const float* values );
    void Write( const std::string variableName, const double* values );
    void Write( const std::string variableName, const long double* values );

private:

    void Init( );
    void InitCapsules( );
    void InitTransports( );

};


} //end namespace engine
} //end namespace adios





#endif /* DATAMAN_H_ */
