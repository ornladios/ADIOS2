/*
 * SingleBP.h
 *
 *  Created on: Dec 16, 2016
 *      Author: wfg
 */

#ifndef WRITER_H_
#define WRITER_H_

#include "core/Engine.h"
#include "format/BP1Writer.h"
#include "capsule/Heap.h"


namespace adios
{


class Writer : public Engine
{

public:

    /**
     * Constructor for Writer writes in BP format into a single heap capsule, manages several transports
     * @param name unique name given to the engine
     * @param accessMode
     * @param mpiComm
     * @param method
     * @param debugMode
     */
    Writer( const std::string name, const std::string accessMode, MPI_Comm mpiComm,
            const Method& method, const bool debugMode = false, const unsigned int cores = 1,
            const std::string hostLanguage = "C++" );

    ~Writer( );


    template< class T >
    void WriteVariable( const Group& group, const Var variableName, const Variable<T>& variable )
    {
        auto lf_CheckAllocationResult = []( const int result, const std::string variableName, const int rankMPI )
        {
            if( result == -1 )
                throw std::runtime_error( "ERROR: bad_alloc when writing variable " + variableName +
                        " from rank " + std::to_string( rankMPI ) );
        };

        //Check if data in buffer needs to be reallocated
        const size_t indexSize = m_BP1Writer.GetVariableIndexSize( group, variableName, variable ); //metadata size
        const std::size_t payloadSize = GetTotalSize( group.GetDimensions( variable.DimensionsCSV ) ) * sizeof( T );
        const std::size_t dataSize = payloadSize + indexSize + 10; //adding some bytes tolerance
        const std::size_t neededSize = dataSize + m_Buffer.m_DataPosition;
        // might need to write payload in batches
        const bool doTransportsFlush = ( neededSize > m_MaxBufferSize )? true : false;

        int result = GrowBuffer( m_MaxBufferSize, m_GrowthFactor, m_Buffer.m_DataPosition, m_Buffer.m_Data );
        lf_CheckAllocationResult( result, variableName, m_RankMPI );

        //WRITE INDEX//
        m_BP1Writer.WriteVariableIndex( group, variableName, variable, m_Buffer, m_MetadataSet );

        if( doTransportsFlush == true ) //in batches
        {

        }
        else //Write data
        {
            //this is the expensive part might want to use threaded memcpy
            MemcpyThreads( m_Buffer.m_Data.data(), variable.Values, payloadSize, m_Cores );
            m_Buffer.m_DataPosition += payloadSize;
            m_Buffer.m_DataAbsolutePosition += payloadSize;
        }
    }

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

    void Close( const int transportIndex = -1 );

private:

    Heap m_Buffer; ///< heap capsule
    format::BP1Writer m_BP1Writer; ///< format object will provide the required BP functionality to be applied on m_Buffer and m_Transports
    format::BP1MetadataSet m_MetadataSet; ///< metadata set accompanying the heap buffer data in bp format. Needed by m_BP1Writer
    std::size_t m_MaxBufferSize;
    float m_GrowthFactor = 1.5;

    void Init( );
    void InitTransports( );

};


} //end namespace adios


#endif /* WRITER_H_ */
