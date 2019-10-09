/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosString.h string manipulation functionality
 *
 *  Created on: May 17, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_HELPER_ADIOSSTRING_H_
#define ADIOS2_HELPER_ADIOSSTRING_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <map>
#include <set>
#include <string>
#include <vector>
/// \endcond

#include "adios2/common/ADIOSTypes.h"

namespace adios2
{
namespace helper
{

/**
 * Opens and checks for file and dumps content to a single string.
 * @param fileName of text file
 * @param hint exception message
 * @return file contents in a string
 */
std::string FileToString(const std::string &fileName, const std::string hint);

/**
 * Transforms a vector to a map of parameters
 * @param parameters vector of parameters with format "field=value"
 * @param debugMode true=check parameters format, false=no checks
 * @return a map with unique key=field, value=corresponding value
 */
Params BuildParametersMap(const std::vector<std::string> &parameters,
                          const char delimKeyValue = '=',
                          const bool debugMode = false);

/**
 * Transforms a string to a map of parameters
 * @param parameters string of parameters with format "key=value,
 * key2=value2, ..."
 * @param debugMode true=check parameters format, false=no checks
 * @return a map with unique key/value pairs
 */
Params BuildParametersMap(const std::string &input,
                          const char delimKeyValue = '=',
                          const char delimItem = ',',
                          const bool debugMode = false);

/**
 * Add name extension if not existing at the end of name
 * @param name input
 * @param extension .ext .bp
 * @return if name already has extension returns name (name.bp), otherwise
 * returns name.extension (name.bp)
 */
std::string AddExtension(const std::string &name,
                         const std::string extension) noexcept;

/**
 * Check if a string ends with another substring
 * @param str input string
 * @param ending input string to compare with
 * @param caseSensitive input flag
 * @return true if the 'str' string ends with the string 'ending'
 */
bool EndsWith(const std::string &str, const std::string &ending,
              const bool caseSensitive = true);

/**
 * Get values for each param entry of a certain key in a vector.
 * If key not found then string in vector is empty.
 * @param key parameter to be extracted
 * @param parametersVector
 * @return vector of values, from key,value in parametersVector
 */
std::vector<std::string>
GetParametersValues(const std::string &key,
                    const std::vector<Params> &parametersVector) noexcept;

/**
 * Searches key and assign value from parameters map
 * @param field input to look for in parameters
 * @param parameters map with key: field, value: value
 * @param value if found it's modified to value in parameters
 */
void SetParameterValue(const std::string key, const Params &parameters,
                       std::string &value) noexcept;

std::string GetParameter(const std::string key, const adios2::Params &params,
                         const bool isMandatory, const bool debugMode,
                         const std::string hint);
/**
 * Sets int value if found in parameters for input key
 * @param key input
 * @param parameters map with key: field, value: value
 * @param value to be modified if key is found in parameters
 * @param debugMode check for string conversion
 * @param hint passed for extra debugging info if exception is thrown
 */
void SetParameterValueInt(const std::string key, const Params &parameters,
                          int &value, const bool debugMode,
                          const std::string &hint);

template <class T>
void SetParameterValue(const std::string key, const Params &parameters,
                       T &value, const bool debugMode, const std::string &hint);

/**
 * Returns a single string with dimension values
 * @param dimensions input
 * @return string dimensions values
 */
std::string DimsToString(const Dims &dimensions);

Dims StringToDims(const std::string &dimensions);

/**
 * Sets global name: prefix + separator + localName. If prefix is empty it
 * returns the localName as-is.
 * e.g.  "temperature" + "/" + "units"
 * @param localName
 * @param prefix
 * @param separator
 * @return 1) prefix + separator + localName, 2) empty prefix: returns localName
 */
std::string GlobalName(const std::string &localName, const std::string &prefix,
                       const std::string separator) noexcept;

/**
 * function that casts a string to a fixed width type verifying validity of the
 * cast with exceptions in debugMode. ONly int32_t, uint32_t, int64_t, uint64_t,
 * float, double are supported
 * @param input to be converted
 * @param debugMode check for string conversion
 * @param hint passed for extra debugging info if exception is thrown
 * @return cast input string to output value of type T
 */
template <class T>
T StringTo(const std::string &input, const bool debugMode,
           const std::string &hint);

/**
 * function that casts a string to a size_t value safely. Calls uint32_t or
 * uint64_t depending on sizeof(size_t)
 * @param input to be converted
 * @param debugMode check for string conversion
 * @param hint passed for extra debugging info if exception is thrown
 * @return cast input string to output value of type size_t
 */
size_t StringToSizeT(const std::string &input, const bool debugMode,
                     const std::string &hint);

/**
 * Convert a string (e.g. 16Kb) to byte units. Last 2 characters must
 * @param input
 * @param debugMode
 * @param hint
 * @return
 */
size_t StringToByteUnits(const std::string &input, const bool debugMode,
                         const std::string &hint);

/**
 * Transforms string to LowerCase
 * @param input string
 * @return input contents in lower case
 */
std::string LowerCase(const std::string &input);

/**
 * Extract inputs subset that matches a prefix input
 * @param prefix input prefix
 * @param inputs input names
 * @return  all names with prefix "pre": pre1, pre2, ..., preXYZ,
 */
std::set<std::string>
PrefixMatches(const std::string &prefix,
              const std::set<std::string> &inputs) noexcept;

} // end namespace helper
} // end namespace adios2

#endif /* ADIOS2_HELPER_ADIOSSTRING_H_ */
