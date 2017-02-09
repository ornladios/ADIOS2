/*
 * DataMan.h
 *
 *  Created on: Jan 10, 2017
 *      Author: wfg
 */

#ifndef VIS_H_
#define VIS_H_


#include "core/Engine.h"
#include "capsule/Heap.h"
#include "format/BP1Writer.h"


namespace adios
{
namespace engine
{


class Vis : public Engine
{

public:

    /**
     * Constructor for single BP capsule engine, writes in BP format into a single heap capsule
     * @param name unique name given to the engine
     * @param accessMode "w" or "write", "r" or "read", "a" or "append"
     * @param mpiComm communicator used for MPI operations
     * @param method contains Engine metadata options provide by the user, Vis can make decisions based on these "knobs"
     * @param debugMode true: handle exceptions, false: skip extra exceptions checks
     * @param hostLanguage for Fortran users (due to array index), most of the time will be set with C++
     */
    Vis( const std::string name, const std::string accessMode, MPI_Comm mpiComm,
         const Method& method, const bool debugMode = false, const unsigned int cores = 1,
         const std::string hostLanguage = "C++" );

    ~Vis( );

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

    void Close( const int transportID = -1 );

private:

    std::vector< std::shared_ptr<Capsule> > m_Capsules; ///< it can be any derived class from Capsule: Heap, Shmem, RDMA ?
    std::size_t m_MaxBufferSize; ///< maximum buffer size
    float m_GrowthFactor = 1.5; ///< buffer growth factor if using a Heap capsule. New_size = f * Previous_size

    //optional if BP format is required
    format::BP1Writer m_BP1Writer; ///< format object will provide the required BP functionality to be applied on m_Buffer and m_Transports
    format::BP1MetadataSet m_MetadataSet; ///< metadata set accompanying the heap buffer data in bp format. Needed by m_BP1Writer

    void Init( );  ///< calls InitTransports based on Method and can extend other Init functions, called from constructor
    void InitTransports( ); ///< from Transports

};


} //end namespace engine
} //end namespace adios





#endif /* VIS_H_ */
