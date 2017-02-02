/*
 * DataMan.h
 *
 *  Created on: Jan 10, 2017
 *      Author: wfg
 */

#ifndef DATAMAN_H_
#define DATAMAN_H_


#include "core/Engine.h"
#include "capsule/Heap.h"
#include "format/BP1Writer.h"


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

    Heap m_Buffer; ///< heap capsule, contains data and metadata buffers
    format::BP1Writer m_BP1Writer; ///< format object will provide the required BP functionality to be applied on m_Buffer and m_Transports

    void Init( );  ///< calls InitCapsules and InitTransports based on Method, called from constructor
    void InitTransports( ); ///< from Transports

    /**
     * From transport Mdtm in m_Method
     * @param parameter must be an accepted parameter
     * @param mdtmParameters
     * @return value either returns user-defined from "parameter=value" or a default
     */
    std::string GetMdtmParameter( const std::string parameter, const std::map<std::string,std::string>& mdtmParameters );

};


} //end namespace engine
} //end namespace adios





#endif /* DATAMAN_H_ */
