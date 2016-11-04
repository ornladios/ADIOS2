/*
ri * GroupFunctions.h helper functions for class CGroup
 *
 *  Created on: Oct 27, 2016
 *      Author: wfg
 */

#ifndef GROUPFUNCTIONS_H_
#define GROUPFUNCTIONS_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <string>
#include <map>
#include <memory> //unique_ptr
/// \endcond

#ifdef HAVE_MPI
  #include <mpi.h>
#else
  #include "public/mpidummy.h"
#endif

#include <core/CVariable.h>
#include "core/CTransport.h"


namespace adios
{

/**
 * Create a language (C++, C, Fortran, etc.) supported type variable and add it to variables map
 * @param name key in variables map
 * @param type variable type, must be in SSupport::Datatypes[hostLanguage] in public/SSupport.h
 * @param dimensionsCSV comma separated variable local dimensions (e.g. "Nx,Ny,Nz")
 * @param transform method, format = lib or lib:level, where lib = zlib, bzip2, szip, and level=1:9
 * @param globalDimensionsCSV comma separated variable global dimensions (e.g. "gNx,gNy,gNz"), if globalOffsetsCSV is also empty variable is local
 * @param globalOffsetsCSV comma separated variable global dimensions (e.g. "gNx,gNy,gNz"), if globalOffsetsCSV is also empty variable is local
 * @param variables map belonging to class CGroup
 */
void CreateVariableLanguage( const std::string name, const std::string type,
                             const std::string dimensionsCSV, const std::string transform,
                             const std::string globalDimensionsCSV, const std::string globalOffsetsCSV,
                             std::map<std::string, std::shared_ptr<CVariableBase> >& variables ) noexcept;


/**
 * Looks up the variable type and cast the appropriate values type to m_Value in CVariable
 * @param variables always a derived CVariable object from CVariableBase
 * @param values to be casted to the right type
 */
void SetVariableValues( CVariableBase& variable, const void* values ) noexcept;



/**
 * Create a derived (child) class of CTransport in transport unique_ptr
 * @param method supported values in SSupport.h TransportMethods
 * @param priority numeric priority for the I/O to schedule this write with others that might be pending
 * @param iteration iterations between writes of a group to gauge how quickly this data should be evacuated from the compute node
 * @param mpiComm MPI communicator from User->ADIOS->Group
 * @param transport passed from CGroup m_Transport member
 */
void CreateTransport( const std::string method, const unsigned int priority, const unsigned int iteration,
                      const MPI_Comm mpiComm, const bool debugMode, std::shared_ptr<CTransport>& transport ) noexcept;



} //end namespace

#endif /* GROUPFUNCTIONS_H_ */
