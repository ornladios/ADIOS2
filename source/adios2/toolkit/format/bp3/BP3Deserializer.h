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

#include <mutex>
#include <set>

#include "adios2/core/IO.h"
#include "adios2/helper/adiosFunctions.h" //VariablesSubFileInfo
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
     * Unique constructor
     * @param mpiComm
     * @param debug true: extra checks
     */
    BP3Deserializer(MPI_Comm mpiComm, const bool debugMode);

    ~BP3Deserializer() = default;

    void ParseMetadata(IO &io);

    // Sync functions
    template <class T>
    std::map<std::string, SubFileInfoMap>
    GetSyncVariableSubFileInfo(const Variable<T> &variable) const;

    // Deferred functions
    template <class T>
    void GetDeferredVariable(Variable<T> &variable, T *data);

    std::map<std::string, SubFileInfoMap>
    PerformGetsVariablesSubFileInfo(IO &io);

    template <class T>
    void ClipContiguousMemory(const std::vector<char> &contiguousMemory,
                              const Box<Dims> &intersectionBox,
                              Variable<T> &variable);

private:
    std::map<std::string, SubFileInfoMap> m_DeferredVariables;

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

    template <class T>
    SubFileInfoMap GetSubFileInfo(const Variable<T> &variable) const;
};

#define declare_template_instantiation(T)                                      \
    extern template std::map<std::string, SubFileInfoMap>                      \
    BP3Deserializer::GetSyncVariableSubFileInfo(const Variable<T> &variable)   \
        const;                                                                 \
                                                                               \
    extern template void BP3Deserializer::GetDeferredVariable(                 \
        Variable<T> &variable, T *data);

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_FORMAT_BP1_BP3DESERIALIZER_H_ */
