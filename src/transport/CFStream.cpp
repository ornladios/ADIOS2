/*
 * CFStream.cpp
 *
 *  Created on: Oct 24, 2016
 *      Author: wfg
 */

#include <iostream>

#include "transport/CFStream.h"
#include "core/CVariableTemplate.h"
#include "core/CVariable.h"


namespace adios
{


CFStream::CFStream( const unsigned int priority, const unsigned int iteration, MPI_Comm mpiComm ):
    CTransport( "CFStream", priority, iteration, mpiComm )
{ }


CFStream::~CFStream( )
{ }


void CFStream::Write( const CVariable& variable )
{
    int rank, size;
    MPI_Comm_rank( m_MPIComm, &rank );
    MPI_Comm_size( m_MPIComm, &size );

    std::cout << "Just saying Hello from CFStream Write from process " << rank << "/" << size  << "\n";
    std::cout << "My variable type is " << variable.m_Type << "\n";

    auto var = variable.Get< std::vector<int> >();

//    std::cout << "Var is empty: " << std::boolalpha << var.empty() << "\n";

    std::cout << "var " << var->at(0) << "\n";

    //pointer to vector
//    for( unsigned int i = 0; i < 10; ++i )
//        std::cout << "var[" << i << "] = " << var->at(i) << "\n";

}



} //end namespace

