/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * PrintData.h
 *
 *  Created on: Apr 2017
 *      Author: Norbert Podhorszki
 */

#ifndef HELLOINSITUPRINT_H_
#define HELLOINSITUPRINT_H_

#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

#include <adios2.h>

void printDataStep(const float *data, const adios2::Dims &size,
                   const adios2::Dims &offset, const int rank, const int step)
{
    std::ofstream myfile;
    std::string filename = "data." + std::to_string(rank);
    if (step == 0)
    {
        myfile.open(filename);
    }
    else
    {
        myfile.open(filename, std::ios::app);
    }

    uint64_t nelems = size[0] * size[1];
    myfile << "rank=" << rank << " size=" << size[0] << "x" << size[1]
           << " offsets=" << offset[0] << ":" << offset[1] << " step=" << step
           << std::endl;

    myfile << " step   row   columns " << offset[1] << "..."
           << offset[1] + size[1] - 1 << std::endl;
    myfile << "        ";
    for (size_t j = 0; j < size[1]; j++)
    {
        myfile << std::setw(9) << offset[1] + j;
    }
    myfile << std::endl;
    myfile << "------------------------------------------------------------"
              "--\n";
    for (size_t i = 0; i < size[0]; i++)
    {
        myfile << std::setw(5) << step << std::setw(5) << offset[0] + i;
        for (size_t j = 0; j < size[1]; j++)
        {
            myfile << std::setw(9) << std::setprecision(5)
                   << data[i * size[1] + j];
        }
        myfile << std::endl;
    }
    data += nelems;
    myfile.close();
}

#endif /* HELLOINSITUPRINT_H_ */
