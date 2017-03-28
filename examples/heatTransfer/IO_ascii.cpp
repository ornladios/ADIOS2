/*
 * IO_ascii.cpp
 *
 *  Created on: Feb 2017
 *      Author: Norbert Podhorszki
 */

#include <iostream>
#include <iomanip>
#include <fstream>

#include "IO.h"

static std::ofstream of;
static std::streambuf *buf;

IO::IO( const Settings& s, MPI_Comm comm )
{
    m_outputfilename = s.outputfile;

    if (m_outputfilename == "cout")
    {
        buf = std::cout.rdbuf();
    }
    else
    {
        int rank;
        MPI_Comm_rank( comm, &rank );
        std::string rs = std::to_string(rank);
        of.open(m_outputfilename + rs + ".txt");
        buf = of.rdbuf();
    }
}

IO::~IO()
{
    if (m_outputfilename != "cout")
    {
        of.close();
    }
}

void IO::write( int step, const HeatTransfer& ht, const Settings& s, MPI_Comm comm)
{
    std::ostream out(buf);
    if( step == 0)
    {
        out << "rank=" << s.rank
                  << " size=" << s.ndx << "x" << s.ndy
                  << " offsets=" << s.offsx << ":" << s.offsy
                  << " step=" << step << std::endl;
        out << " time   row   columns " << s.offsy << "..." << s.offsy+s.ndy-1 << std::endl;
        out << "        ";
        for (int j = 1; j <= s.ndy; ++j) {
            out <<  std::setw(9) << s.offsy+j-1;
        }
        out << "\n--------------------------------------------------------------\n";
    }
    else
    {
        out << std::endl;
    }

    for (int i = 1; i <= s.ndx; ++i)
    {
        out <<  std::setw(5) << step << std::setw(5) << s.offsx+i-1;
        for (int j = 1; j <= s.ndy; ++j)
        {
            out <<  std::setw(9) << ht.T(i,j);
        }
        out << std::endl;
    }

}

