/*
 * ADIOS.h
 *
 *  Created on: Oct 3, 2016
 *      Author: wfg
 */

#ifndef ADIOS_H_
#define ADIOS_H_

#include <string>
#include <memory>

#ifdef HAVE_MPI
#include <mpi.h>
#endif

#include "SMetadata.h"
#include "CFile.h"


namespace adios
{
/**
 * Class ADIOS, interface between user call and ADIOS library
 */
class ADIOS
{

public: // can be accessed by anyone

    std::string m_XMLConfigFile; ///< XML File to be read containing configuration information
    bool m_IsUsingMPI = false; ///< bool flag false: sequential, true: parallel MPI, to be checked instead of communicator
    std::unique_ptr<CFile> m_File;
    SMetadata m_Metadata; ///< contains all metadata information from XML Config File

    ADIOS( ); ///< empty constructor, to be defined. Since we have an empty constructor we can't have const variables not defined by this constructor

    /**
     * Serial constructor for XML config file
     * @param xmlConfigFile passed to m_XMLConfigFile
     */
    ADIOS( const std::string xmlConfigFile );

    #ifdef HAVE_MPI //START OF THE MPI only section
    MPI_Comm m_MPIComm = nullptr; ///< only used as reference to MPI communicator passed from parallel constructor, using pointer instead of reference as null is a possible value
    /**
     * Parallel constructor for XML config file and MPI
     * @param xmlConfigFile passed to m_XMLConfigFile
     * @param mpiComm MPI communicator ...const to be discussed
     */
    ADIOS( const std::string xmlConfigFile, const MPI_Comm mpiComm );

    /**
     * Parallel MPI communicator without XML config
     * @param mpiComm MPI communicator passed to m_MPIComm ...const to be discussed
     */
    //ADIOS( const MPI_Comm mpiComm );
    #endif  //END OF THE MPI ONLY SECTION

    ~ADIOS( ); ///< virtual destructor overriden by children's own destructors

    void Init( ); ///< calls to read XML file among other initialization tasks

private:

    void InitNoMPI( ); ///< called from Init, initialize Serial

    #ifdef HAVE_MPI
    void InitMPI( ); ///< called from Init, initialize parallel MPI
    #endif

    void ReadXMLConfigFile( ); ///< populates SMetadata by reading the XML Config File
};



} //end namespace




#endif /* ADIOS_H_ */
