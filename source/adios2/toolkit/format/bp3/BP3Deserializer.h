/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP3Deserializer.h
 *
 *  Created on: Sep 7, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP1_BP3DESERIALIZER_H_
#define ADIOS2_TOOLKIT_FORMAT_BP1_BP3DESERIALIZER_H_

#include <map>
#include <mutex>
#include <string>
#include <utility> //std::pair

#include "adios2/core/IO.h"
#include "adios2/toolkit/format/bp3/BP3Base.h"

namespace adios2
{
namespace format
{

class BP3Deserializer : public BP3Base
{

public:
    /** BP Minifooter fields */
    Minifooter m_Minifooter;

    /**
     * Synchronous Reads
     * <pre>
     * key: subfile index
     * value: map
     *        key: step
     *        value: bucket index vector of pairs -> pair.first = seekStart,
     *                                               pair.second = seekCount
     * </pre>
     */
    std::map<unsigned int,
             std::map<size_t, std::vector<std::pair<size_t, size_t>>>>
        m_VariableSubFileInfo;

    /**
     * Deferred Reads
     * <pre>
     *
     * key: subfile index
     * value: map
     *        key: variable name
     *        value: map
     *               key: step
     *               value: bucket index vector of pairs:
     *                                                   pair.first = seekStart,
     *                                                   pair.second = seekCount
     * </pre>
     */
    std::map<unsigned int,         // file index
             std::map<std::string, // variable name, step, seekStart, seekCount
                      std::map<size_t, std::vector<std::pair<size_t, size_t>>>>>
        m_MultiVariablesSubFileInfo;

    /**
     * Unique constructor
     * @param mpiComm
     * @param debug true: extra checks
     */
    BP3Deserializer(MPI_Comm mpiComm, const bool debugMode);

    ~BP3Deserializer() = default;

    void ParseMetadata(IO &io);

private:
    static std::mutex m_Mutex;

    void ParseMinifooter();
    void ParsePGIndex();
    void ParseVariablesIndex(IO &io);
    void ParseAttributesIndex(IO &io);

    /**
     * This function reads a variable index element (serialized) and calls IO
     * DefineVariable to deserialize the data
     * @param header serialize
     * @param io
     * @param buffer
     * @param position
     */
    template <class T>
    void DefineVariableInIO(const ElementIndexHeader &header, IO &io,
                            const std::vector<char> &buffer,
                            size_t position) const;
};

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_FORMAT_BP1_BP3DESERIALIZER_H_ */
