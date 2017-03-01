/*
 * IO.h
 *
 *  Created on: Feb 2017
 *      Author: Norbert Podhorszki
 */

#ifndef IO_H_
#define IO_H_

#include <mpi.h>
#include "Settings.h"
#include "HeatTransfer.h"

class IO
{

public:
    IO( std::shared_ptr<Settings> s, MPI_Comm comm );
    ~IO();
    void write( int step, std::shared_ptr<HeatTransfer> ht, std::shared_ptr<Settings> s, MPI_Comm comm );
    
private:
    std::string m_outputfilename;
};

#endif /* IO_H_ */
