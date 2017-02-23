/*
 * BPWriter.h
 *
 *  Created on: Dec 16, 2016
 *      Author: wfg
 */

#ifndef BPWRITER_H_
#define BPWRITER_H_

#include "core/Engine.h"
#include "format/BP1Writer.h"

//supported capsules
#include "capsule/heap/STLVector.h"


namespace adios
{

class BPWriter : public Engine
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
    BPWriter( ADIOS& adios, const std::string name, const std::string accessMode, MPI_Comm mpiComm,
              const Method& method, const bool debugMode = false, const unsigned int cores = 1 );

    ~BPWriter( );

    void Write( Variable<char>& variable, const char* values );
    void Write( Variable<unsigned char>& variable, const unsigned char* values );
    void Write( Variable<short>& variable, const short* values );
    void Write( Variable<unsigned short>& variable, const unsigned short* values );
    void Write( Variable<int>& variable, const int* values );
    void Write( Variable<unsigned int>& variable, const unsigned int* values );
    void Write( Variable<long int>& variable, const long int* values );
    void Write( Variable<unsigned long int>& variable, const unsigned long int* values );
    void Write( Variable<long long int>& variable, const long long int* values );
    void Write( Variable<unsigned long long int>& variable, const unsigned long long int* values );
    void Write( Variable<float>& variable, const float* values );
    void Write( Variable<double>& variable, const double* values );
    void Write( Variable<long double>& variable, const long double* values );
    void Write( Variable<std::complex<float>>& variable,       const std::complex<float>* values );
    void Write( Variable<std::complex<double>>& variable,      const std::complex<double>* values );
    void Write( Variable<std::complex<long double>>& variable, const std::complex<long double>* values );
    void Write( VariableCompound& variable, const void* values );

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
    void Write( const std::string variableName, const std::complex<float>* values );
    void Write( const std::string variableName, const std::complex<double>* values );
    void Write( const std::string variableName, const std::complex<long double>* values );
    void Write( const std::string variableName, const void* values );

    void AdvanceStep( );

    void Close( const int transportIndex = -1 );

private:

    capsule::STLVector m_Buffer; ///< heap capsule using STL std::vector<char>
    std::size_t m_BufferVariableCountPosition = 0; ///< needs to be updated in every advance step
    bool m_IsFirstClose = true; ///< set to false after first Close is reached so metadata doesn't have to be accommodated for a subsequent Close
    std::size_t m_MaxBufferSize;
    float m_GrowthFactor = 1.5; ///< capsule memory growth factor, new_memory = m_GrowthFactor * current_memory

    format::BP1Writer m_BP1Writer; ///< format object will provide the required BP functionality to be applied on m_Buffer and m_Transports
    format::BP1MetadataSet m_MetadataSet; ///< metadata set accompanying the heap buffer data in bp format. Needed by m_BP1Writer

    bool m_TransportFlush = false; ///< true: transport flush happened, buffer must be reset

    void Init( );
    void InitTransports( );
    void InitProcessGroup( );


    void WriteProcessGroupIndex( );


    /**
     * Common function for primitive (including std::complex) writes
     * @param group
     * @param variableName
     * @param variable
     */
    template< class T >
    void WriteVariableCommon( Variable<T>& variable, const T* values )
    {
        //set variable
        variable.m_AppValues = values;
        m_WrittenVariables.insert( variable.m_Name );

        //precalculate new metadata and payload sizes
        const std::size_t indexSize = m_BP1Writer.GetVariableIndexSize( variable );
        const std::size_t payloadSize = variable.PayLoadSize(); //will change if compression is applied
        //Buffer reallocation, expensive part
        m_TransportFlush = CheckBuffersAllocation( indexSize, payloadSize );

        //WRITE INDEX to data buffer and metadata structure (in memory)//
        m_BP1Writer.WriteVariableIndex( variable, m_Buffer, m_MetadataSet );

        if( m_TransportFlush == true ) //in batches
        {
            //write pg index

            //flush to transports

            //reset positions to zero, update absolute position

        }
        else //Write data to buffer
        {
            m_BP1Writer.WriteVariablePayload( variable, m_Buffer, m_Cores );
        }
    }

    /**
     * Check if heap buffers for data and metadata need reallocation or maximum sizes have been reached.
     * @param indexSize precalculated index size
     * @param payloadSize payload size from variable total size
     * @return true: transport must be flush and buffers reset, false: buffer is sufficient
     */
    bool CheckBuffersAllocation( const std::size_t indexSize, const std::size_t payloadSize );

};


} //end namespace adios


#endif /* BPWRITER_H_ */
