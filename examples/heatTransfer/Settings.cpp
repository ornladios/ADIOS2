/*
 * Settings.cpp
 *
 *  Created on: Feb 2017
 *      Author: Norbert Podhorszki
 */



#include <errno.h>
#include <cstdlib>

#include "Settings.h"


static unsigned int convertToUint( std::string varName, char *arg )
{
    char *end;
    int retval = std::strtoll( arg, &end, 10);
    if( end[0] || errno == ERANGE )
    {
        throw std::invalid_argument( "Invalid value given for " + varName + ": "
                + std::string(arg) );
    }
    if( retval < 0 )
    {
        throw std::invalid_argument( "Negative value given for " + varName + ": "
                + std::string(arg) );
    }
    return (unsigned int) retval;
}

Settings::Settings( int argc, char* argv [], int rank, int nproc )
: rank{rank}
{
    if (argc < 8)
    {
        throw std::invalid_argument( "Not enough arguments" );
    }
    this->nproc = (unsigned int) nproc;

    outputfile = argv[1];
    npx = convertToUint("N", argv[2]);
    npy = convertToUint("M", argv[3]);
    ndx = convertToUint("nx", argv[4]);
    ndy = convertToUint("ny", argv[5]);
    steps = convertToUint("steps", argv[6]);
    iterations = convertToUint("iterations", argv[7]);

    if( npx * npy != this->nproc )
    {
        throw std::invalid_argument( "N*M must equal the number of processes" );
    }

    // calculate global array size and the local offsets in that global space
    gndx  = npx * ndx;
    gndy  = npy * ndy;
    posx  = rank % npx;
    posy  = rank / npx;
    offsx = posx * ndx;
    offsy = posy * ndy;

    // determine neighbors
    if( posx == 0 )
        rank_left = -1;
    else
        rank_left = rank-1;

    if( posx == npx-1 )
        rank_right = -1;
    else
        rank_right = rank+1;

    if( posy == 0 )
        rank_up = -1;
    else
        rank_up = rank - npx;

    if( posy == npy-1 )
        rank_down = -1;
    else
        rank_down = rank + npx;

}

Settings::~Settings()
{
}

