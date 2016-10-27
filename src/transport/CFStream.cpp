/*
 * CFStream.cpp
 *
 *  Created on: Oct 24, 2016
 *      Author: wfg
 */


#include <iostream>

#include "transport/CFStream.h"
#include "core/CVariable.h" // implements CVariableBase Get function


namespace adios
{


CFStream::CFStream( const unsigned int priority, const unsigned int iteration, MPI_Comm mpiComm ):
    CTransport( "CFStream", priority, iteration, mpiComm )
{ }


CFStream::~CFStream( )
{ }


void CFStream::Write( const CVariableBase& variable )
{
    int rank, size;
    MPI_Comm_rank( m_MPIComm, &rank );
    MPI_Comm_size( m_MPIComm, &size ); //would write to file

    const std::string type( variable.m_Type );
    std::cout << "My variable type is " << variable.m_Type << "\n";

    if( type[0] == 'V' ) //meaning it's a vector
    {
        //pointer to vector
        auto values = variable.Get< std::vector<int> >();
        std::cout << "Vector of size " <<  values->size() << "\n";
        unsigned int i = 0;
        for( auto element : *values )
        {
            std::cout << "var[" << i << "] = " << element << "\n";
            ++i;
        }
    }
}



} //end namespace

