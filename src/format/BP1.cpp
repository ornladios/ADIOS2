/*
 * BP1.cpp
 *
 *  Created on: Feb 7, 2017
 *      Author: wfg
 */


#include "format/BP1.h"
#include "functions/adiosFunctions.h"


namespace adios
{
namespace format
{


void BP1::OpenRankFiles( const std::string name, const std::string accessMode, Transport& file )
{
    std::string directory;
    std::string baseName;

    if( name.find(".bp") == name.size()-3 ) //need to test
    {
        baseName = name.substr( 0, name.size()-3 );
        directory = name + ".dir";
        CreateDirectory( name +".dir" );
    }
    else
    {
        baseName = name;
        directory = name + ".bp.dir";
        CreateDirectory( name +".bp.dir" );
    }

    std::string fileName( directory + "/" + baseName + ".bp." + std::to_string( file.m_MPIRank ) );

    if( file.m_MPIComm == MPI_COMM_SELF )
        fileName = name;

    file.Open( fileName, accessMode );  // opens a file transport under name.bp.dir/name.bp.rank reserve that location fro writing
}



} //end namespace format
} //end namespace adios
