/*
 * adiosFunctions.h Long helper functions used by ADIOS class
 *
 *  Created on: Oct 10, 2016
 *      Author: wfg
 */

#ifndef ADIOSFUNCTIONS_H_
#define ADIOSFUNCTIONS_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <string>
#include <vector>
#include <map>
#include <cstring> //std::size_t
/// \endcond

#ifdef HAVE_MPI
  #include <mpi.h>
#else
  #include "mpidummy.h"
#endif


#include "core/Group.h"


namespace adios
{

/**
 * Opens and checks for file and dumps contents to a single string
 * @param fileName file to be opened
 * @param fileContents output contains the entire file
 */
void DumpFileToString( const std::string fileName, std::string& fileContents );


/**
 * Extracts a substring between two tags from content
 * @param initialTag
 * @param finalTag
 * @param content full string
 * @param subString if found return substring between initialTag and finalTag, otherwise returns empty
 * @param currentPosition to start the search, moved forward to finalTag position
 */
void GetSubString ( const std::string initialTag, const std::string finalTag, const std::string content, std::string& subString,
                    std::string::size_type& currentPosition );

/**
 * Extracts the value inside quotes in a string currentTag ( Example: currentTag --> field1="value1" field2="value2" )
 * @param quote double " or single '
 * @param quotePosition position of the opening quote in currentTag
 * @param currentTag initial tag value, modified by cutting the first found " " portion, currentTag --> field2="value2"
 * @param value value1 in the example above
 */
void GetQuotedValue( const char quote, const std::string::size_type& quotePosition,
                     std::string& currentTag, std::string& value );


/**
 * Get attributes field1="value1" field2="value2" by looping through a single XML tag
 * @param tag field0="value0" field1="value1" in a single string
 * @param pairs pairs[0].first=field0 pairs[0].second=value0 pairs[1].first=field1 pairs[1].second=value1
 */
void GetPairs( const std::string tag, std::vector< std::pair<const std::string, const std::string> >& pairs ) noexcept;


/**
 * Determine tag type and call GetPairs to populate pairs
 * @param fileContent file Content in a single string
 * @param tag field0="value0" field1="value1" in a single string
 * @param pairs pairs[0].first=field0 pairs[0].second=value0 pairs[1].first=field1 pairs[1].second=value1
 */
void GetPairsFromTag( const std::string& fileContent, const std::string tag,
                      std::vector< std::pair<const std::string, const std::string> >& pairs );


/**
 * Set members m_Groups and m_HostLanguage from XML file content, called within Init functions
 * @param fileContent file Content in a single string
 * @param mpiComm MPI Communicator passed from application passed to Transport method if required
 * @param hostLanguage return the host language from fileContent
 * @param transforms return the modified transforms vector if there are variables with transformations
 * @param groups passed returns the map of groups defined in fileContent
 */
void SetMembers( const std::string& fileContent, const MPI_Comm mpiComm,
                 std::string& hostLanguage, std::vector< std::shared_ptr<Transform> >& transforms,
                 std::map< std::string, Group >& groups );


/**
 * Called inside the ADIOS XML constructors to get contents from file, broadcast and set hostLanguage and groups from ADIOS class
 * @param xmlConfigFile xml config file name
 * @param mpiComm communicator used from broadcasting
 * @param debugMode from ADIOS m_DebugMode passed to CGroup in groups
 * @param hostLanguage set from host-language in xml file
 * @param transforms return the modified transforms vector if there are variables with transformations
 * @param groups passed returns the map of groups defined in fileContent
 */
void InitXML( const std::string xmlConfigFile, const MPI_Comm mpiComm, const bool debugMode,
              std::string& hostLanguage, std::vector< std::shared_ptr<Transform> >& transforms,
              std::map< std::string, Group >& groups );


/**
 * Loops through a vector containing dimensions and returns the product of all elements
 * @param dimensions input containing size on each dimension {Nx, Ny, Nz}
 * @return product of all dimensions Nx * Ny * Nz
 */
unsigned long long int GetTotalSize( const std::vector<unsigned long long int>& dimensions );


/**
 * Might need to add exceptions for debug mode
 * Creates a chain of directories using POSIX systems calls (stat, mkdir),
 * Verifies if directory exists before creating a new one. Permissions are 777 for now
 * @param fullPath /full/path/for/directory
 */
void CreateDirectory( const std::string fullPath ) noexcept;


/**
 * Identifies, verifies the corresponding transform method and adds it the transforms container if neccesary.
 * This functions must be updated as new transform methods are supported.
 * @param variableTransforms methods to be added to transforms with format "method:compressionLevel", or  "method" with compressionLevel=0 (default)
 * @param transforms container of existing transform methods, owned by ADIOS class
 * @param debugMode if true will do more checks, exceptions, warnings, expect slower code
 * @param transformIndices returns the corresponding indices in ADIOS m_Transforms for a single variable
 * @param parameters returns the corresponding parameters understood by a collection of transform="method:parameter"
 */
void SetTransformsHelper( const std::vector<std::string>& transformNames, std::vector< std::shared_ptr<Transform> >& transforms,
                          const bool debugMode, std::vector<short>& transformIndices, std::vector<short>& parameters );


/**
 * Transforms a vector
 * @param parameters vector of parameters with format "field=value"
 * @param debugMode true=check parameters format, false=no checks
 * @return a map with unique key=field, value=corresponding value
 */
std::map<std::string, std::string> BuildParametersMap( const std::vector<std::string>& parameters, const bool debugMode );


/**
 * Single call that extract data buffers information from Capsule. That way virtual Capsule functions are called a few times
 * @param capsules input
 * @param dataBuffers from Capsule.GetData()
 * @param positions
 * @param absolutePositions
 */
//void GetDataBuffers( const std::vector<Capsule*>& capsules, std::vector<char*>& dataBuffers, std::vector<std::size_t>& positions,
//                     std::vector<std::size_t>& absolutePositions );

/**
 * Converts comma-separated values to a vector of integers
 * @param csv "1,2,3"
 * @return vector<int> = { 1, 2, 3 }
 */
std::vector<int> CSVToVectorInt( const std::string csv );


/**
 * Grows a buffer by a factor of  n . growthFactor . currentCapacity to accommodate for incomingDataSize
 * @param incomingDataSize size of new data required to be stored in buffer
 * @param growthFactor buffer grows in multiples of the growth buffer
 * @param currentPosition current buffer position, needed to get available space = currentCapacity - currentPosition
 * @param buffer to be resized
 * @return -1: failed to allocate (bad_alloc), 0: didn't have to allocate (enough space), 1: successful allocation
 */
int GrowBuffer( const std::size_t incomingDataSize, const float growthFactor, const std::size_t currentPosition,
                std::vector<char>& buffer );

/**
 * Moves positions in a vector by a number of bytes
 * @param bytes input number of bytes
 * @param positions  += bytes
 */
void MovePositions( const int bytes, std::vector<std::size_t>& positions );

} //end namespace



#endif /* ADIOSFUNCTIONS_H_ */
