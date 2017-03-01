/*
 * IO_ADIOS2.cpp
 *
 *  Created on: Feb 2017
 *      Author: Norbert Podhorszki
 */

#include "IO.h"


IO::IO( const Settings& s, MPI_Comm comm )
{
    m_outputfilename = s.outputfile + ".bp";
}

IO::~IO()
{
}

void IO::write(int step, const HeatTransfer& ht, const Settings& s, MPI_Comm comm )
{
}

