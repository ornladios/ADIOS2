/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * PrintData.h
 *
 *  Created on: Apr 2017
 *      Author: Norbert Podhorszki
 */

#ifndef PRINTDATA_H_
#define PRINTDATA_H_

#include <cstdint>

void printData(double *xy, size_t *size, size_t *offset, int rank, int steps);

#endif /* PRINTDATA_H_ */
