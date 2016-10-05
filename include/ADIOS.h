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

#include "CFile.h"


namespace adios
{
/**
 * @brief Unique class interface between user application and ADIOS library
 */
class ADIOS
{

public: // can be accessed by anyone

    /**
     * ADIOS empty constructor. Used for non XML calls.
     */
    ADIOS( );

    /**
     * @brief Serial constructor for XML config file
     * @param xmlConfigFile passed to m_XMLConfigFile
     */
    ADIOS( const std::string xmlConfigFile );

  // ******************************************************************************* /
  // START OF THE MPI only members section ******************************************/
  // ******************************************************************************* /
  #ifdef HAVE_MPI
    /**
     * @brief Parallel constructor for XML config file and MPI
     * @param xmlConfigFile passed to m_XMLConfigFile
     * @param mpiComm MPI communicator ...const to be discussed
     */
    ADIOS( const std::string xmlConfigFile, const MPI_Comm mpiComm );

    /**
     * Parallel MPI communicator without XML config
     * @param mpiComm MPI communicator passed to m_MPIComm
     */
    ADIOS( const MPI_Comm mpiComm );
  #endif
  // *END****************************************************************************** /



    void Open( const std::string fileName, const std::string groupName, const std::string accessMode );



    ~ADIOS( ); ///< virtual destructor overriden by children's own destructors

    void Init( ); ///< calls to read XML file among other initialization tasks



private:

    std::string m_XMLConfigFile; ///< XML File to be read containing configuration information
    bool m_IsUsingMPI = false; ///< bool flag false: sequential, true: parallel MPI, to be checked instead of communicator
  #ifdef HAVE_MPI
    MPI_Comm m_MPIComm = nullptr; ///< only used as reference to MPI communicator passed from parallel constructor, MPI_Comm is a pointer itself
  #endif

    std::string HostLanguage; ///< Supported languages: C, C++, Fortran

    /**
     * @brief List of groups defined for ADIOS configuration (XML).
     * <pre>
     * \t Key: std::string unique group name
     * \t Value: SGroup struct in SGroup.h
     * </pre>
     */
    std::map< std::string, SGroup > Groups;

    /**
     * @brief Maximum buffer size in ADIOS write() operation. From buffer max - size - MB in XML file
     * Note, that if there are two ADIOS outputs going on at the same time,
     * ADIOS will allocate two separate buffers each with the specified maximum limit.
     */
    unsigned int MaxBufferSizeInMB;

    void InitNoMPI( ); ///< called from Init, initialize Serial

  #ifdef HAVE_MPI
    void InitMPI( ); ///< called from Init, initialize parallel MPI
  #endif

    void ReadXMLConfigFile( ); ///< populates SMetadata by reading the XML Config File

};



} //end namespace




#endif /* ADIOS_H_ */
