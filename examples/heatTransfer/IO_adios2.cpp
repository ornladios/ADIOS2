/*
 * IO_ADIOS2.cpp
 *
 *  Created on: Feb 2017
 *      Author: Norbert Podhorszki
 */

#include "IO.h"


IO::IO( std::string output_basename )
{
    m_outputfilename = output_basename + ".bp";
}

IO::~IO()
{
}

void IO::write( int step, int curr, Settings& s )
{
}

