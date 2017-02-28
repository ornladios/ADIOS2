/*
 * Settings.h
 *
 *  Created on: Feb 2017
 *      Author: Norbert Podhorszki
 */

#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <memory>

class Settings
{

public:
    // user arguments
    std::string outputfile;
    int npx; // Number of processes in X (slow) dimension
    int npy; // Number of processes in Y (fast) dimension
    int ndx; // Local array size in X dimension per process
    int ndy; // Local array size in y dimension per process
    int steps; // Number of output steps
    int iterations; // Number of computing iterations between steps

    // calculated values from those arguments and number of processes
    int gndx; // Global array size in slow dimension
    int gndy; // Global array size in fast dimension
    // X dim positions: rank 0, npx, 2npx... are in the same X position
    // Y dim positions: npx number of consecutive processes belong to one row (npx columns)
    int posx; // Position of this process in X dimension
    int posy; // Position of this process in Y dimension
    int offsx; // Offset of local array in X dimension on this process
    int offsy; // Offset of local array in Y dimension on this process

    int rank;  // MPI rank
    int nproc; // number of processors

    // neighbours by their MPI ranks
    int rank_left;
    int rank_right;
    int rank_up;
    int rank_down;

    Settings( int argc, char* argv [], int rank, int nproc );
    ~Settings();

};


#endif /* SETTINGS_H_ */
