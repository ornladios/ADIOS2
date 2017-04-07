/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * HeatTransfer.h
 *
 *  Created on: Feb 2017
 *      Author: Norbert Podhorszki
 */

#ifndef HEATTRANSFER_H_
#define HEATTRANSFER_H_

#include <mpi.h>

#include <vector>

#include "Settings.h"

class HeatTransfer
{
public:
    HeatTransfer(const Settings &settings); // Create two 2D arrays with ghost
                                            // cells to compute
    ~HeatTransfer();
    void init(bool init_with_rank); // set up array values with either rank or
                                    // real demo values
    void iterate();                 // one local calculation step
    void heatEdges();               // reset the heat values at the global edge
    void exchange(MPI_Comm comm);   // send updates to neighbors

    // return a single value at index i,j. 0 <= i <= ndx+2, 0 <= j <= ndy+2
    double T(int i, int j) const { return m_TCurrent[i][j]; };
    // return (1D) pointer to current T data, ndx+2 * ndy+2 elements
    double *data() const { return m_TCurrent[0]; };
    // return (1D) pointer to current T data without ghost cells, ndx*ndy
    // elements
    std::vector<double> data_noghost() const;

    void printT(std::string message,
                MPI_Comm comm) const; // debug: print local TCurrent on stdout

private:
    const double edgetemp = 3.0; // temperature at the edges of the global plate
    const double omega =
        0.8;       // weight for current temp is (1-omega) in iteration
    double **m_T1; // 2D array (ndx+2) * (ndy+2) size, including ghost cells
    double **m_T2; // another 2D array
    double **m_TCurrent; // pointer to T1 or T2
    double **m_TNext;    // pointer to T2 or T1
    const Settings &m_s;
    void switchCurrentNext(); // switch the current array with the next array
};

#endif /* HEATTRANSFER_H_ */
