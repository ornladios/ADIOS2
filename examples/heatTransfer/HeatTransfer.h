/*
 * HeatTransfer.h
 *
 *  Created on: Feb 2017
 *      Author: Norbert Podhorszki
 */

#ifndef HEATTRANSFER_H_
#define HEATTRANSFER_H_

#include <mpi.h>
#include "Settings.h"

class HeatTransfer
{
public:
    HeatTransfer( std::shared_ptr<Settings> settings ); // Create two 2D arrays with ghost cells to compute
    ~HeatTransfer();
    void init(bool init_with_rank); // set up array values with either rank or real demo values
    void iterate();  // one local calculation step
    void heatEdges(); // reset the heat values at the global edge
    void exchange( MPI_Comm comm ); // send updates to neighbors

    const double *data() {return m_TCurrent[0];}; // return (1D) pointer to current T data
    const double T(int i, int j) {return m_TCurrent[i][j];}; // return current T value at i,j local coordinate

    void printT(std::string message, MPI_Comm comm); // debug: print local TCurrent on stdout

private:
    const double edgetemp = 3.0; // temperature at the edges of the global plate
    const double omega = 0.8;    // weight for current temp is (1-omega) in iteration
    double **m_T1;       // 2D array (ndx+2) * (ndy+2) size, including ghost cells
    double **m_T2;       // another 2D array
    double **m_TCurrent; // pointer to T1 or T2
    double **m_TNext;    // pointer to T2 or T1
    std::shared_ptr<Settings> m_s;
    void switchCurrentNext(); // switch the current array with the next array
};

#endif /* HEATTRANSFER_H_ */
