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
    bool m_TransportFlush = false; ///< true: transport flush happened, buffer must be reset

    void Init( );
    void InitTransports( );


    /**
     * Common function
     * @param group
     * @param variableName
     * @param variable
     */
    template< class T >
    void WriteVariable( const Group& group, const Var variableName, const Variable<T>& variable )
    {
        //precalculate new metadata and payload sizes
        const std::size_t indexSize = m_BP1Writer.GetVariableIndexSize( group, variableName, variable );
        const std::size_t payloadSize = GetTotalSize( group.GetDimensions( variable.DimensionsCSV ) ) * sizeof( T );

        //Buffer reallocation, expensive part
        m_TransportFlush = CheckBuffersAllocation( group, variableName, indexSize, payloadSize );

        //WRITE INDEX to data buffer and metadata structure (in memory)//
        m_BP1Writer.WriteVariableIndex( group, variableName, variable, m_Buffer, m_MetadataSet );

        if( m_TransportFlush == true ) //in batches
        {
            //write pg index

            //flush to transports

            //reset positions to zero, update absolute position

        }
        else //Write data to buffer
        {
            //Values to Buffer -> Copy of data, Expensive part might want to use threads if large. Need a model to apply threading.
            MemcpyThreads( m_Buffer.m_Data.data(), variable.Values, payloadSize, m_Cores );
            //update indices
            m_Buffer.m_DataPosition += payloadSize;
            m_Buffer.m_DataAbsolutePosition += payloadSize;
        }
    }

    /**
     * Check if heap buffers for data and metadata need reallocation or maximum sizes have been reached.
     * @param group variable owner
     * @param variableName name of the variable to be written
     * @param indexSize precalculated index size
     * @param payloadSize payload size from variable total size
     * @return true: transport must be flush and buffers reset, false: buffer is sufficient
     */
    bool CheckBuffersAllocation( const Group& group, const Var variableName, const std::size_t indexSize, const std::size_t payloadSize );

};


} //end namespace adios


#endif /* WRITER_H_ */
