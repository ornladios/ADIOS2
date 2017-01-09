/*
 * Capsule.h
 *
 *  Created on: Dec 7, 2016
 *      Author: wfgtemplates and pointers
 */

#ifndef CAPSULE_H_
#define CAPSULE_H_

#include "core/Variable.h"


namespace adios
{

/**
 * Base class that raw data and metadata buffers, used by Engine.
 * Derived classes will allocate their own buffer in different memory spaces.
 * e.g. locally (heap) or in shared memory (virtual memory)
 */
class Capsule
{

public:

    const std::string m_Type; ///< buffer type
    const std::string m_AccessMode; ///< 'w': write, 'r': read, 'a': append

    /**
     * Base class constructor providing type from derived class and accessMode
     * @param type derived class type
     * @param accessMode 'w':write, 'r':read, 'a':append
     * @param rankMPI current MPI rank
     * @param cores if using threads
     */
    Capsule( const std::string type, const std::string accessMode, const int rankMPI,
             const bool debugMode, const unsigned int cores );


    virtual ~Capsule( );

    virtual char* GetData( ) = 0; ///< return the pointer to the raw data buffer
    virtual char* GetMetadata( ) = 0; ///< return the pointer to the raw metadata buffer

    virtual const std::size_t GetDataSize( ) const = 0; ///< get current data buffer size
    virtual const std::size_t GetMetadataSize( ) const = 0; ///< get current metadata buffer size

    virtual void ResizeData( const std::size_t size ); ///< resize data buffer
    virtual void ResizeMetadata( const std::size_t size ); ///< resize metadata buffer

    /**
     * Write functions to data buffer
     * @param first position for writing in data buffer
     * @param data incoming data variable
     * @param size number of elements
     */
    virtual void WriteData( const std::size_t first, const char* data, const std::size_t size ) = 0;
    virtual void WriteData( const std::size_t first, const unsigned char* data, const std::size_t size ) = 0;
    virtual void WriteData( const std::size_t first, const short* data, const std::size_t size ) = 0;
    virtual void WriteData( const std::size_t first, const unsigned short* data, const std::size_t size ) = 0;
    virtual void WriteData( const std::size_t first, const int* data, const std::size_t size ) = 0;
    virtual void WriteData( const std::size_t first, const unsigned int* data, const std::size_t size ) = 0;
    virtual void WriteData( const std::size_t first, const long int* data, const std::size_t size ) = 0;
    virtual void WriteData( const std::size_t first, const unsigned long int* data, const std::size_t size ) = 0;
    virtual void WriteData( const std::size_t first, const long long int* data, const std::size_t size ) = 0;
    virtual void WriteData( const std::size_t first, const unsigned long long int* data, const std::size_t size ) = 0;
    virtual void WriteData( const std::size_t first, const float* data, const std::size_t size ) = 0;
    virtual void WriteData( const std::size_t first, const double* data, const std::size_t size ) = 0;
    virtual void WriteData( const std::size_t first, const long double* data, const std::size_t size ) = 0;

    virtual void WriteMetadata( const std::size_t first, const char* metadata, const std::size_t size ) = 0;
    virtual void WriteMetadata( const std::size_t first, const unsigned char* metadata, const std::size_t size ) = 0;
    virtual void WriteMetadata( const std::size_t first, const short* metadata, const std::size_t size ) = 0;
    virtual void WriteMetadata( const std::size_t first, const unsigned short* metadata, const std::size_t size ) = 0;
    virtual void WriteMetadata( const std::size_t first, const int* metadata, const std::size_t size ) = 0;
    virtual void WriteMetadata( const std::size_t first, const unsigned int* metadata, const std::size_t size ) = 0;
    virtual void WriteMetadata( const std::size_t first, const long int* metadata, const std::size_t size ) = 0;
    virtual void WriteMetadata( const std::size_t first, const unsigned long int* metadata, const std::size_t size ) = 0;
    virtual void WriteMetadata( const std::size_t first, const long long int* metadata, const std::size_t size ) = 0;
    virtual void WriteMetadata( const std::size_t first, const unsigned long long int* metadata, const std::size_t size ) = 0;
    virtual void WriteMetadata( const std::size_t first, const float* metadata, const std::size_t size ) = 0;
    virtual void WriteMetadata( const std::size_t first, const double* metadata, const std::size_t size ) = 0;
    virtual void WriteMetadata( const std::size_t first, const long double* metadata, const std::size_t size ) = 0;


protected:

    const int m_RankMPI = 0; ///< current MPI rank
    const bool m_DebugMode = false; ///< true: extra checks
    const unsigned int m_Cores = 1; ///< number of cores for threaded operations

};



} //end namespace

#endif /* CAPSULE_H_ */
