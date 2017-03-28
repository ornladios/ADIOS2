/*
 * IO.h
 *
 *  Created on: Feb 2017
 *      Author: Norbert Podhorszki
 */

#ifndef IO_H_
#define IO_H_

#include "HeatTransfer.h"
#include "Settings.h"
#include <mpi.h>

class IO
{

public:
  IO(const Settings &s, MPI_Comm comm);
  ~IO();
  void write(int step, const HeatTransfer &ht, const Settings &s,
             MPI_Comm comm);

private:
  std::string m_outputfilename;
};

#endif /* IO_H_ */
