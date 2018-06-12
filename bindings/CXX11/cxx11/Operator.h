/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Operator.h :
 *
 *  Created on: Jun 7, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_CXX11_CXX11_OPERATOR_H_
#define ADIOS2_BINDINGS_CXX11_CXX11_OPERATOR_H_

#include "adios2/ADIOSMacros.h"
#include "adios2/ADIOSTypes.h"

namespace adios2
{

// forward declare
class ADIOS; // friend class
class IO;    // friend class

template <class T>
class Variable; // friend class

namespace core
{
class Operator; // private implementation
}

class Operator
{
    friend class ADIOS;
    friend class IO;

    template <class T>
    friend class Variable;

public:
    ~Operator() = default;

    /** true: valid object, false: invalid object */
    explicit operator bool() const noexcept;

    std::string Type() const noexcept;

    void SetParameter(const std::string key, const std::string value);

    const Params &GetParameters();

    template <class T>
    void RunCallback1(const T *, const std::string &, const std::string &,
                      const std::string &, const Dims &);

    void RunCallback2(void *, const std::string &, const std::string &,
                      const std::string &, const Dims &);

    /**
     * Returns a conservative buffer size to hold input data for classes
     * @param sizeIn size of input data to be compressed in bytes
     * @return recommended allocation for output buffer
     */
    size_t BufferMaxSize(const size_t sizeIn) const;

    /**
     * Used by Zfp
     * Returns a conservative buffer size to hold input data for classes
     * @param dataIn
     * @param dimensions
     * @return recommended allocation for output buffer in bytes
     */
    template <class T>
    size_t BufferMaxSize(const T *dataIn, const Dims &dimensions,
                         const Params &parameters) const;

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
    template <class T>
    size_t Compress(const T *dataIn, const Dims &dimensions,
                    const size_t elementSize, const std::string type,
                    char *bufferOut, const Params &parameters = Params()) const;

    /**
     * BZip2 signature
     * @param bufferIn
     * @param sizeIn
     * @param dataOut
     * @param sizeOut
     * @return size of decompressed buffer
     */
    template <class T>
    size_t Decompress(const char *bufferIn, const size_t sizeIn, T *dataOut,
                      const size_t sizeOut) const;

    /**
     * Zfp signature
     * @param bufferIn
     * @param sizeIn
     * @param dataOut
     * @param dimensions
     * @param type
     * @return size of decompressed buffer
     */
    template <class T>
    size_t Decompress(const char *bufferIn, const size_t sizeIn, T *dataOut,
                      const Dims &dimensions, const std::string type,
                      const Params &parameters) const;

private:
    Operator(core::Operator *op);
    core::Operator *m_Operator = nullptr;
};

#define declare_template_instantiation(T)                                      \
    extern template void Operator::RunCallback1(                               \
        const T *, const std::string &, const std::string &,                   \
        const std::string &, const Dims &);                                    \
                                                                               \
    extern template size_t Operator::Compress(const T *, const Dims &,         \
                                              const size_t, const std::string, \
                                              char *, const Params &) const;   \
                                                                               \
    extern template size_t Operator::Decompress(const char *, const size_t,    \
                                                T *, const size_t) const;

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

#define declare_template_instantiation(T)                                      \
    extern template size_t Operator::BufferMaxSize(const T *, const Dims &,    \
                                                   const Params &) const;      \
                                                                               \
    extern template size_t Operator::Decompress(                               \
        const char *, const size_t, T *, const Dims &, const std::string,      \
        const Params &) const;

ADIOS2_FOREACH_ZFP_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace adios2

#endif /* ADIOS2_BINDINGS_CXX11_CXX11_OPERATOR_H_ */
