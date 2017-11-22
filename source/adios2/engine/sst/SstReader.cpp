/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SstReader.cpp
 *
 *  Created on: Aug 17, 2017
 *      Author: Greg Eisenhauer
 */

#include "SstReader.h"

namespace adios2
{

SstReader::SstReader(IO &io, const std::string &name, const Mode mode,
                     MPI_Comm mpiComm)
: Engine("SstReader", io, name, mode, mpiComm)
{
    SstStream output;
    char *cstr = new char[name.length() + 1];
    strcpy(cstr, name.c_str());

    m_Input = SstReaderOpen(cstr, NULL, mpiComm);
    Init();
    delete[] cstr;
}

void SstReader::Close(const int transportIndex) { SstReaderClose(m_Input); }

// PRIVATE
void SstReader::Init()
{
    auto itRealTime = m_IO.m_Parameters.find("real_time");
}

} // end namespace adios
