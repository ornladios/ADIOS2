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

/**
 * @brief Abstract Parent class for all different File types ADIOS implements.
 */
class CFile
{

public:

    const std::string m_FileName; ///< serial: file name, MPI: prefix for file name
    const std::string m_FileType; ///< netCDF, PHDF5, POSIX
    const bool m_IsUsingMPI; ///< true: Communicator is passed, false: serial process (although it might include threads)

    /**
     * Only this constructor is allowed
     * @param fileName passed to m_FileName
     * @param fileType
     * @param metadata
     * @param isUsingMPI
     */
    CFile( const std::string fileName, const std::string fileType, const bool isUsingMPI ):
        m_FileName{ fileName },
        m_FileType{ fileType },
        m_IsUsingMPI{ isUsingMPI }
    { }

    /**
     * Empty destructor, using STL
     */
    virtual ~CFile( )
    { };

    /**
     * Open operation
     * @param fileName
     * @param groupName
     * @param accessMode
     */
    virtual void Open( const std::string fileName, const std::string groupName, const std::string accessMode ) = 0;

    virtual unsigned long int GroupSize( const std::string fileName, const std::string groupName ) const = 0;

    virtual void Write( const std::string fileName, const std::string variableName ) = 0;

    virtual void Close( ) = 0;

};


} //end namespace



#endif /* CFILE_H_ */
