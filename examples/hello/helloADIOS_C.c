/*
 * helloADIOS_C.cpp
 *
 *  Created on: Oct 31, 2016
 *      Author: wfg
 */

#ifdef HAVE_MPI
  #include <mpi.h>
#else
  #include "../../include/mpidummy.h"
#endif

#include "../../include/ADIOS_C.h"



int main( int argc, char* argv [] )
{
    MPI_Init( &argc, &argv );
    int rank;
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );









    MPI_Finalize( );


    return 0;
}
