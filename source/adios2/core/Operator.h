/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Operator.h : Base class for all derive operators classes under
 * adios2/operator.
 * This include callback functions, compression, etc.
 *
 *  Created on: Oct 17, 2016
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_CORE_OPERATOR_H_
#define ADIOS2_CORE_OPERATOR_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <functional>
#include <string>
#include <vector>
/// \endcond

#include "adios2/common/ADIOSMacros.h"
#include "adios2/common/ADIOSTypes.h"

namespace adios2
{
namespace core
{

class Operator
{

public:
    /** From derived class */
    const std::string m_Type;

    /**
     * Base class constructor
     * @param type from derived class object: e.g. bzip2, zfp, callback
     * @param debugMode true: extra exceptions checks
     */
    Operator(const std::string type, const Params &parameters,
             const bool debugMode);

    virtual ~Operator() = default;

    void SetParameter(const std::string key, const std::string value) noexcept;

    Params &GetParameters() noexcept;

#define declare_type(T)                                                        \
    virtual void RunCallback1(const T *, const std::string &,                  \
                              const std::string &, const std::string &,        \
                              const size_t, const Dims &, const Dims &,        \
                              const Dims &) const;
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

    virtual void RunCallback2(void *, const std::string &, const std::string &,
                              const std::string &, const size_t, const Dims &,
                              const Dims &, const Dims &) const;
    /**
     * Returns a conservative buffer size to hold input data for classes
     * @param sizeIn size of input data to be compressed in bytes
     * @return recommended allocation for output buffer
     */
    virtual size_t BufferMaxSize(const size_t sizeIn) const;

    /**
     * Used by Zfp
     * Returns a conservative buffer size to hold input data for classes
     * @param dataIn
     * @param dimensions
     * @return recommended allocation for output buffer in bytes
     */
    template <class T>
    size_t BufferMaxSize(const T *dataIn, const Dims &dimensions,
                         const Params &params) const;

    /**
     * BZip2 and Zfp common call
     * @param dataIn
     * @param dimensions
     * @param elementSize
     * @param type
     * @param bufferOut
     * @param parameters
     * @return size of compressed buffer
     */
    virtual size_t Compress(const void *dataIn, const Dims &dimensions,
                            const size_t elementSize, const std::string type,
                            void *bufferOut, const Params &parameters,
                            Params &info) const;

    virtual size_t Decompress(const void *bufferIn, const size_t sizeIn,
                              void *dataOut, const size_t sizeOut,
                              Params &info) const;

    /**
     * Zfp signature
     * @param bufferIn
     * @param sizeIn
     * @param dataOut
     * @param dimensions
     * @param type
     * @return
     */
    virtual size_t Decompress(const void *bufferIn, const size_t sizeIn,
                              void *dataOut, const Dims &dimensions,
                              const std::string type,
                              const Params &parameters) const;

protected:
    /** Parameters associated with a particular Operator */
    Params m_Parameters;

    /** true: extra exception checks, false: skip exception checks */
    const bool m_DebugMode = false;

    /**
     * Used by CompressZfp
     * Returns a conservative buffer size to hold input data for classes
     * @param dataIn
     * @param dimensions
     * @param type
     * @return conservative buffer size for allocation
     */
    virtual size_t DoBufferMaxSize(const void *dataIn, const Dims &dimensions,
                                   const std::string type,
                                   const Params &parameters) const;

private:
    void CheckCallbackType(const std::string type) const;
};

} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_CORE_OPERATOR_H_ */
