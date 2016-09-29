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



namespace adios
{
/**
 * Parent abstract class CADIOS, Children must implement this class virtual functions.
 */
class CADIOS
{

public:

	std::string m_XMLConfigFile; ///< XML File to be read containing configuration information

	CADIOS( ); ///< empty constructor, to be defined. Since we have an empty constructor we can't have const variables not defined by this constructor

	/**
	 * Serial constructor for XML config file
	 * @param xmlConfigFile passed to m_XMLConfigFile
	 */
	CADIOS( const std::string xmlConfigFile );

	/**
	 * Parallel constructor for XML config file and MPI
	 * @param xmlConfigFile passed to m_XMLConfigFile
	 * @param mpiCommunicator MPI communicator ...const to be discussed
	 */
	CADIOS( const std::string xmlConfigFile, const MPI_Comm& mpiComm );

	virtual ~CADIOS( ); ///< virtual destructor overriden by children's own destructor

	void Init( ); ///< calls to read XML file


protected: // only children can access this

	std::unique_ptr<MPI_Comm> m_MPIComm; ///< reference to MPI communicator passed from MPI constructor

	void ReadXMLConfigFile( ); ///< Must be implemented in CADIOS.cpp common to all classes


};







}

#endif /* CADIOS_H_ */
