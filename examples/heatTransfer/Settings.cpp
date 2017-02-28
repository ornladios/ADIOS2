/*
 * Settings.cpp
 *
 *  Created on: Feb 2017
 *      Author: Norbert Podhorszki
 */



#include <errno.h>
#include <cstdlib>

#include "Settings.h"


static int convertToInt( std::string varName, char *arg )
{
    char *end;
    int retval = std::strtoll( arg, &end, 10);
    if( end[0] || errno == ERANGE )
    {
        throw std::invalid_argument( "Invalid value given for " + varName + ": "
                + std::string(arg) );
    }
    return retval;
}

Settings::Settings( int argc, char* argv [], int rank, int nproc )
: rank{rank}, nproc{nproc}
{
    if (argc < 8)
    {
        throw std::invalid_argument( "Not enough arguments" );
    }

    outputfile = argv[1];
    npx = convertToInt("N", argv[2]);
    npy = convertToInt("M", argv[3]);
    ndx = convertToInt("nx", argv[4]);
    ndy = convertToInt("ny", argv[5]);
    steps = convertToInt("steps", argv[6]);
    iterations = convertToInt("iterations", argv[7]);

    if( npx * npy != nproc )
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

