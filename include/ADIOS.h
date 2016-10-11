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

#include "SGroup.h"
#include "SSupport.h"


namespace adios
{
/**
 * @brief Unique class interface between user application and ADIOS library
 */
class ADIOS
{

public: // PUBLIC Constructors and Functions define the User Interface with ADIOS

    std::map< std::string, SGroup > m_Groups;
    /**
     * @brief ADIOS empty constructor. Used for non XML config file API calls.
     */
    ADIOS( );

    /**
     * @brief Serial constructor for XML config file
     * @param xmlConfigFile passed to m_XMLConfigFile
     */
    ADIOS( const std::string xmlConfigFile);

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
     * @brief Parallel MPI communicator without XML config file
     * @param mpiComm MPI communicator passed to m_MPIComm
     */
    ADIOS( const MPI_Comm mpiComm );
  #endif
  // *END****************************************************************************** /

    ~ADIOS( ); ///< virtual destructor overriden by children's own destructors

    void Init( ); ///< calls to read XML file among other initialization tasks
    /**
     * @brief Open or Append to an output file
     * @param groupName should match an existing group from XML file or created through CreateGroup
     * @param fileName associated file
     * @param accessMode "w": write, "a": append, need more info on this
     */
    void Open( const std::string groupName, const std::string fileName, const std::string accessMode = "w" );

    /**
     * @brief Get the total sum of payload and overhead, which includes name, data type, dimensions and other metadata
     * @param groupName
     * @return group size in
     */
    unsigned long int GroupSize( const std::string groupName ) const;


    /**
     * Submits a data element values for writing and associates it with the given variableName
     * @param variableName name of existing scalar or vector variable in the XML file or created with CreateVariable
     * @param values pointer to the variable values passed from the user application, use dynamic_cast to check that pointer is of the same value type
     */
    template<class T>
    void Write( const std::string groupName, const std::string variableName, const T* values );

    void Close( ); // dumps to file?


private:

    std::string m_XMLConfigFile; ///< XML File to be read containing configuration information
    bool m_IsUsingMPI = false; ///< bool flag false: sequential, true: parallel MPI, to be checked instead of communicator
  #ifdef HAVE_MPI
    MPI_Comm m_MPIComm = nullptr; ///< only used as reference to MPI communicator passed from parallel constructor, MPI_Comm is a pointer itself
  #endif

    std::string m_HostLanguage; ///< Supported languages: C, C++, Fortran

    /**
     * @brief List of groups defined for ADIOS configuration (XML).
     * <pre>
     *     Key: std::string unique group name
     *     Value: SGroup struct in SGroup.h
     * </pre>
     */


    /**
     * @brief Maximum buffer size in ADIOS write() operation. From buffer max - size - MB in XML file
     * Note, that if there are two ADIOS outputs going on at the same time,
     * ADIOS will allocate two separate buffers each with the specified maximum limit.
     * Default = 0 means there is no limit
     */
    unsigned int MaxBufferSizeInMB = 0;

    void InitNoMPI( ); ///< called from Init, initialize Serial

  #ifdef HAVE_MPI
    void InitMPI( ); ///< called from Init, initialize parallel MPI
  #endif
};



} //end namespace




#endif /* ADIOS_H_ */
