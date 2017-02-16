/*
 * DataMan.h
 *
 *  Created on: Jan 10, 2017
 *      Author: wfg
 */

#ifndef DATAMAN_H_
#define DATAMAN_H_

#include <iostream> //must be removed
#include <unistd.h> //must be removed

#include "core/Engine.h"
#include "capsule/Heap.h"
#include "format/BP1Writer.h"


namespace adios
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
     * @param hostLanguage
     */
    DataMan( ADIOS& adios, const std::string name, const std::string accessMode, MPI_Comm mpiComm,
             const Method& method, const bool debugMode = false, const unsigned int cores = 1 );

    ~DataMan( );

    void Write( Variable<char>& variable, const char* values );
    void Write( Variable<unsigned char>& variable, const unsigned char* values );
    void Write( Variable<short>& variable, const short* values );
    void Write( Variable<unsigned short>& variable, const unsigned short* values );
    void Write( Variable<int>& variable, const int* values );
    void Write( Variable<unsigned int>& variable, const unsigned int* values );
    void Write( Variable<long int>& variable, const long int* values );
    void Write( Variable<unsigned long int>& variable, const unsigned long int* values );
    void Write( Variable<long long int>& variable, const long long int* values );
    void Write( Variable<unsigned long long int>& variable, const unsigned long long int* values ) ;
    void Write( Variable<float>& variable, const float* values );
    void Write( Variable<double>& variable, const double* values );
    void Write( Variable<long double>& variable, const long double* values );

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
    void InitCapsules( );
    void InitTransports( ); ///< from Transports

    /**
     * From transport Mdtm in m_Method
     * @param parameter must be an accepted parameter
     * @param mdtmParameters
     * @return value either returns user-defined from "parameter=value" or a default
     */
    std::string GetMdtmParameter( const std::string parameter, const std::map<std::string,std::string>& mdtmParameters );


    template<class T>
    void WriteVariable( Variable<T>& variable, const T* values )
    {
        //here comes your magic at Writting now variable.m_UserValues has the data passed by the user
        //set variable
        variable.m_AppValues = values;
        m_WrittenVariables.insert( variable.m_Name );

        //This part will go away, this is just to monitor variables per rank
        MPI_Barrier( m_MPIComm );

        for( int i = 0; i < m_SizeMPI; ++i )
        {
            if( i == m_RankMPI )
            {
                std::cout << "Rank: " << m_RankMPI << "\n";
                variable.Monitor( std::cout );
                std::cout << std::endl;
            }
            else
            {
                sleep( 0.1 );
            }
        }
        MPI_Barrier( m_MPIComm );
    }

};


} //end namespace adios





#endif /* DATAMAN_H_ */
