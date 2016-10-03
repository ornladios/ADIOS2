/*
 * CFile.h
 *
 *  Created on: Oct 3, 2016
 *      Author: wfg
 */

#ifndef CFILE_H_
#define CFILE_H_

#include <string>

#include "SMetadata.h"

namespace adios
{

class CFile
{

public:

    const std::string m_FileName; ///< serial: file name, MPI: prefix for file name
    const std::string m_FileType; ///< netCDF, PHDF5, POSIX
    const bool m_IsUsingMPI; ///< true: Communicator is passed, false: serial process (might include threads)
    const SMetadata& m_Metadata; ///< reference to metadata info

    CFile( const std::string fileName, const std::string fileType, const SMetadata& metadata, const bool isUsingMPI ):
        m_FileName{ fileName },
        m_FileType{ fileType },
        m_Metadata{ metadata },
        m_IsUsingMPI{ isUsingMPI }
    { }

    virtual ~CFile( ) { };

    virtual void Open( const std::string fileName, const std::string groupName, const std::string accessMode ) = 0;

    virtual unsigned long int GroupSize( const std::string fileName, const std::string groupName ) const = 0;

    virtual void Write( const std::string fileName, const std::string variableName ) = 0;

    virtual void Close( ) = 0;

};


} //end namespace



#endif /* CFILE_H_ */
