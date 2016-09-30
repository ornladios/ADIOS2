/*
 * CADIOS.h
 *
 *  Created on: Sep 29, 2016
 *      Author: William F Godoy
 */

#ifndef CADIOS_H_
#define CADIOS_H_

#include <string>
#include <mpi.h>
#include <memory> // unique_ptr

#include "include/SMetadata.h"

namespace adios
{
/**
 * Class CADIOS, interface between user call and ADIOS library
 */
class CADIOS
{

public: // can be accessed by anyone

    std::string m_XMLConfigFile; ///< XML File to be read containing configuration information
    bool m_IsUsingMPI = false; ///< bool flag false: sequential, true: parallel MPI, to be checked instead of communicator
	MPI_Comm* m_MPIComm = nullptr; ///< only used as reference to MPI communicator passed from parallel constructor, using pointer instead of reference as null is a possible value

	CADIOS( ); ///< empty constructor, to be defined. Since we have an empty constructor we can't have const variables not defined by this constructor

	/**
	 * Serial constructor for XML config file
	 * @param xmlConfigFile passed to m_XMLConfigFile
	 */
	CADIOS( const std::string xmlConfigFile );

	/**
	 * Parallel constructor for XML config file and MPI
	 * @param xmlConfigFile passed to m_XMLConfigFile
	 * @param mpiComm MPI communicator ...const to be discussed
	 */
	CADIOS( const std::string xmlConfigFile, const MPI_Comm& mpiComm );

	~CADIOS( ); ///< virtual destructor overriden by children's own destructors

	void Init( ); ///< calls to read XML file among other initialization tasks

private:

	SMetadata m_Metadata; ///< contains all metadata information from XML Config File

	void InitSerial( ); ///< called from Init, initialize Serial
	void InitMPI( ); ///< called from Init, initialize parallel MPI

	void ReadXMLConfigFile( ); ///< populates SMetadata by reading the XML Config File


};







}

#endif /* CADIOS_H_ */
