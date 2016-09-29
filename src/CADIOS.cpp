/*
 * CADIOS.cpp
 *
 *  Created on: Sep 29, 2016
 *      Author: William F Godoy
 *
 */

#include "include/CADIOS.h"


namespace adios
{

//here assign default values
CADIOS::CADIOS( )
{ }


CADIOS::CADIOS( const std::string xmlConfigFile ):
    m_XMLConfigFile( xmlConfigFile )
{ }


CADIOS::CADIOS( const std::string xmlConfigFile, const MPI_Comm& mpiComm  ):
    m_XMLConfigFile( xmlConfigFile )
{ }








}



