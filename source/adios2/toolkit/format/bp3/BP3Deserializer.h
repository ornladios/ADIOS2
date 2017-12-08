/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP3Deserializer.h
 *
 *  Created on: Sep 7, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP3_BP3DESERIALIZER_H_
#define ADIOS2_TOOLKIT_FORMAT_BP3_BP3DESERIALIZER_H_

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

    bool m_PerformedGets = true;

    /**
     * Unique constructor
     * @param mpiComm
     * @param debug true: extra checks
     */
    BP3Deserializer(MPI_Comm mpiComm, const bool debugMode);

    ~BP3Deserializer() = default;

    void ParseMetadata(const BufferSTL &bufferSTL, IO &io);

    // Sync functions
    template <class T>
    std::map<std::string, SubFileInfoMap>
    GetSyncVariableSubFileInfo(const Variable<T> &variable) const;

    /**
     * Used to get the variable payload data for the current selection (dims and
     * steps), used in single buffer for streaming
     * @param variable
     * @param bufferSTL bp buffer input that contains metadata and data
     */
    template <class T>
    void GetSyncVariableDataFromStream(Variable<T> &variable,
                                       BufferSTL &bufferSTL) const;

    // Deferred functions
    template <class T>
    void GetDeferredVariable(Variable<T> &variable, T *data);

    std::map<std::string, SubFileInfoMap>
    PerformGetsVariablesSubFileInfo(IO &io);

    void ClipContiguousMemory(const std::string &variableName, IO &io,
                              const std::vector<char> &contiguousMemory,
                              const Box<Dims> &blockBox,
                              const Box<Dims> &intersectionBox) const;

    void GetStringFromMetadata(Variable<std::string> &variable) const;

private:
    std::map<std::string, SubFileInfoMap> m_DeferredVariables;

    static std::mutex m_Mutex;

    void ParseMinifooter(const BufferSTL &bufferSTL);
    void ParsePGIndex(const BufferSTL &bufferSTL);
    void ParseVariablesIndex(const BufferSTL &bufferSTL, IO &io);
    void ParseAttributesIndex(const BufferSTL &bufferSTL, IO &io);

    /**
     * Reads a variable index element (serialized) and calls IO.DefineVariable
     * to deserialize the Variable metadata
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
    void DefineAttributeInIO(const ElementIndexHeader &header, IO &io,
                             const std::vector<char> &buffer,
                             size_t position) const;

    template <class T>
    SubFileInfoMap GetSubFileInfo(const Variable<T> &variable) const;

    template <class T>
    void ClipContiguousMemoryCommon(Variable<T> &variable,
                                    const std::vector<char> &contiguousMemory,
                                    const Box<Dims> &blockBox,
                                    const Box<Dims> &intersectionBox) const;

    /**
     * Row-major, zero-indexed data e.g. : C, C++
     * @param variable
     * @param contiguousMemory
     * @param blockBox
     * @param intersectionBox
     */
    template <class T>
    void ClipContiguousMemoryCommonRow(
        Variable<T> &variable, const std::vector<char> &contiguousMemory,
        const Box<Dims> &blockBox, const Box<Dims> &intersectionBox) const;

    /**
     * Column-major, one indexed data e.g. : Fortran, R
     * @param variable
     * @param contiguousMemory
     * @param blockBox
     * @param intersectionBox
     */
    template <class T>
    void ClipContiguousMemoryCommonColumn(
        Variable<T> &variable, const std::vector<char> &contiguousMemory,
        const Box<Dims> &blockBox, const Box<Dims> &intersectionBox) const;
};

#define declare_template_instantiation(T)                                      \
    extern template std::map<std::string, SubFileInfoMap>                      \
    BP3Deserializer::GetSyncVariableSubFileInfo(const Variable<T> &variable)   \
        const;                                                                 \
                                                                               \
    extern template void BP3Deserializer::GetSyncVariableDataFromStream(       \
        Variable<T> &variable, BufferSTL &bufferSTL) const;                    \
                                                                               \
    extern template void BP3Deserializer::GetDeferredVariable(                 \
        Variable<T> &variable, T *data);

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_FORMAT_BP3_BP3DESERIALIZER_H_ */
