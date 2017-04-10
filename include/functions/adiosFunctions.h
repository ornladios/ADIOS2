/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosFunctions.h Long helper functions used by ADIOS class
 *
 *  Created on: Oct 10, 2016
 *      Author: wfg
 */

#ifndef ADIOSFUNCTIONS_H_
#define ADIOSFUNCTIONS_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <cstring> //std::size_t
#include <map>
#include <memory> //std::shared_ptr
#include <string>
#include <vector>
/// \endcond

#include "ADIOS_MPI.h"

#include "core/Transform.h"

namespace adios
{

/**
 * Opens and checks for file and dumps contents to a single string
 * @param fileName file to be opened
 * @param fileContents output contains the entire file
 */
void DumpFileToString(const std::string fileName, std::string &fileContents);

/**
 * Extracts a substring between two tags from content
 * @param initialTag
 * @param finalTag
 * @param content full string
 * @param subString if found return substring between initialTag and finalTag,
 * otherwise returns empty
 * @param currentPosition to start the search, moved forward to finalTag
 * position
 */
void GetSubString(const std::string initialTag, const std::string finalTag,
                  const std::string content, std::string &subString,
                  std::string::size_type &currentPosition);

/**
 * Extracts the value inside quotes in a string currentTag ( Example: currentTag
 * --> field1="value1" field2="value2" )
 * @param quote double " or single '
 * @param quotePosition position of the opening quote in currentTag
 * @param currentTag initial tag value, modified by cutting the first found " "
 * portion, currentTag --> field2="value2"
 * @param value value1 in the example above
 */
void GetQuotedValue(const char quote,
                    const std::string::size_type &quotePosition,
                    std::string &currentTag, std::string &value);

/**
 * Get attributes field1="value1" field2="value2" by looping through a single
 * XML tag
 * @param tag field0="value0" field1="value1" in a single string
 * @param pairs pairs[0].first=field0 pairs[0].second=value0
 * pairs[1].first=field1 pairs[1].second=value1
 */
void GetPairs(const std::string tag,
              std::vector<std::pair<const std::string, const std::string>>
                  &pairs) noexcept;

/**
 * Determine tag type and call GetPairs to populate pairs
 * @param fileContent file Content in a single string
 * @param tag field0="value0" field1="value1" in a single string
 * @param pairs pairs[0].first=field0 pairs[0].second=value0
 * pairs[1].first=field1 pairs[1].second=value1
 */
void GetPairsFromTag(
    const std::string &fileContent, const std::string tag,
    std::vector<std::pair<const std::string, const std::string>> &pairs);

/**
 * Set members m_Groups and m_HostLanguage from XML file content, called within
 * Init functions
 * @param fileContent file Content in a single string
 * @param mpiComm MPI Communicator passed from application passed to Transport
 * method if required
 * @param hostLanguage return the host language from fileContent
 * @param transforms return the modified transforms vector if there are
 * variables with transformations
 * @param groups passed returns the map of groups defined in fileContent
 */
// void SetMembers( const std::string& fileContent, const MPI_Comm mpiComm,
//                 std::string& hostLanguage, std::vector<
//                 std::shared_ptr<Transform> >& transforms,
//                 std::map< std::string, Group >& groups );

/**
 * Called inside the ADIOS XML constructors to get contents from file, broadcast
 * and set hostLanguage and groups from ADIOS class
 * @param xmlConfigFile xml config file name
 * @param mpiComm communicator used from broadcasting
 * @param debugMode from ADIOS m_DebugMode passed to CGroup in groups
 * @param hostLanguage set from host-language in xml file
 * @param transforms return the modified transforms vector if there are
 * variables with transformations
 * @param groups passed returns the map of groups defined in fileContent
 */
// void InitXML( const std::string xmlConfigFile, const MPI_Comm mpiComm, const
// bool debugMode,
//              std::string& hostLanguage, std::vector<
//              std::shared_ptr<Transform> >& transforms,
//              std::map< std::string, Group >& groups );

/**
 * Loops through a vector containing dimensions and returns the product of all
 * elements
 * @param dimensions input containing size on each dimension {Nx, Ny, Nz}
 * @return product of all dimensions Nx * Ny * Nz
 */
std::size_t GetTotalSize(const std::vector<size_t> &dimensions);

/**
 * Might need to add exceptions for debug mode
 * Creates a chain of directories using POSIX systems calls (stat, mkdir),
 * Verifies if directory exists before creating a new one. Permissions are 777
 * for now
 * @param fullPath /full/path/for/directory
 */
void CreateDirectory(const std::string fullPath) noexcept;

/**
 * Identifies, verifies the corresponding transform method and adds it the
 * transforms container if neccesary.
 * This functions must be updated as new transform methods are supported.
 * @param variableTransforms methods to be added to transforms with format
 * "method:compressionLevel", or  "method" with compressionLevel=0 (default)
 * @param transforms container of existing transform methods, owned by ADIOS
 * class
 * @param debugMode if true will do more checks, exceptions, warnings, expect
 * slower code
 * @param transformIndices returns the corresponding indices in ADIOS
 * m_Transforms for a single variable
 * @param parameters returns the corresponding parameters understood by a
 * collection of transform="method:parameter"
 */
void SetTransformsHelper(const std::vector<std::string> &transformNames,
                         std::vector<std::shared_ptr<Transform>> &transforms,
                         const bool debugMode,
                         std::vector<short> &transformIndices,
                         std::vector<short> &parameters);

/**
 * Transforms a vector
 * @param parameters vector of parameters with format "field=value"
 * @param debugMode true=check parameters format, false=no checks
 * @return a map with unique key=field, value=corresponding value
 */
std::map<std::string, std::string>
BuildParametersMap(const std::vector<std::string> &parameters,
                   const bool debugMode);

/**
 * Single call that extract data buffers information from Capsule. That way
 * virtual Capsule functions are called a few times
 * @param capsules input
 * @param dataBuffers from Capsule.GetData()
 * @param positions
 * @param absolutePositions
 */
// void GetDataBuffers( const std::vector<Capsule*>& capsules,
// std::vector<char*>& dataBuffers, std::vector<std::size_t>& positions,
//                     std::vector<std::size_t>& absolutePositions );

/**
 * Converts comma-separated values to a vector of integers
 * @param csv "1,2,3"
 * @return vector<int> = { 1, 2, 3 }
 */
std::vector<int> CSVToVectorInt(const std::string csv);

/** Convert a vector of uint64_t elements to a vector of std::size_t elements
 *  @param input vector of uint64_t elements
 *  @param output vector of std::size_t elements
 */
void ConvertUint64VectorToSizetVector(const std::vector<std::uint64_t> &in,
                                      std::vector<std::size_t> &out);
/**
 * Converts a vector of dimensions to a CSV string
 * @param dims vector of dimensions
 * @return comma separate value (CSV)
 */
std::string DimsToCSV(const std::vector<std::size_t> &dims);

/**
 * Common strategy to check for heap buffer allocation for data and metadata
 * typically calculated in Write
 * @param newSize new data size
 * @param growthFactor user provided growth factor for index and data memory
 * buffers ( default = 1.5 )
 * @param maxBufferSize user provided maximum buffer size
 * @param buffer to be reallocated
 * @return true: must do a transport flush, false: buffer sizes are enough
 * to
 * contain incoming data, no need for transport flush
 */
bool CheckBufferAllocation(const std::size_t newSize, const float growthFactor,
                           const std::size_t maxBufferSize,
                           std::vector<char> &buffer);

/**
 * Grows a buffer by a factor of  n . growthFactor . currentCapacity to
 * accommodate for incomingDataSize
 * @param incomingDataSize size of new data required to be stored in buffer
 * @param growthFactor buffer grows in multiples of the growth buffer
 * @param buffer to be resized
 * @return -1: failed to allocate (bad_alloc), 0: didn't have to allocate
 * (enough space), 1: successful allocation
 */
int GrowBuffer(const std::size_t incomingDataSize, const float growthFactor,
               std::vector<char> &buffer);

/**
 * Check if system is little endian
 * @return true: little endian, false: big endian
 */
bool IsLittleEndian() noexcept;

} // end namespace

#endif /* ADIOSFUNCTIONS_H_ */
