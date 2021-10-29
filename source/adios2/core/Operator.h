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

#include "adios2/common/ADIOSMacros.h"
#include "adios2/common/ADIOSTypes.h"
#include <cstring>
#include <functional>

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
     */
    Operator(const std::string type, const Params &parameters);

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
     * @param dataIn
     * @param blockStart
     * @param blockCount
     * @param type
     * @param bufferOut
     * @param parameters
     * @return size of compressed buffer
     */
    virtual size_t Operate(const char *dataIn, const Dims &blockStart,
                           const Dims &blockCount, const DataType type,
                           char *bufferOut, const Params &parameters);

    /**
     * @param bufferIn
     * @param sizeIn
     * @param dataOut
     * @return size of decompressed buffer
     */
    virtual size_t InverseOperate(const char *bufferIn, const size_t sizeIn,
                                  char *dataOut);

    virtual bool IsDataTypeValid(const DataType type) const = 0;

    enum OperatorType : char
    {
        BLOSC = 0,
        BZIP2 = 1,
        LIBPRESSIO = 2,
        MGARD = 3,
        PNG = 4,
        SIRIUS = 5,
        Sz = 6,
        ZFP = 7
    };

protected:
    /** Parameters associated with a particular Operator */
    Params m_Parameters;

    /**
     * Used by lossy compressors with a limitation on complex data types or
     * dimentions Returns a adios2::Dims object that meets the requirement of a
     * compressor
     * @param dimensions
     * @param type
     * @param targetDims
     * @return refined dimensions
     */
    Dims ConvertDims(const Dims &dimensions, const DataType type,
                     const size_t targetDims = 0,
                     const bool enforceDims = false,
                     const size_t defaultDimSize = 1) const;

    template <typename T, typename U>
    void PutParameter(char *buffer, U &pos, const T &parameter)
    {
        std::memcpy(buffer + pos, &parameter, sizeof(T));
        pos += sizeof(T);
    }

    template <typename T, typename U>
    T GetParameter(const char *buffer, U &pos)
    {
        T ret;
        std::memcpy(&ret, buffer + pos, sizeof(T));
        pos += sizeof(T);
        return ret;
    }

    template <typename U>
    void PutParameters(char *buffer, U &pos, const Params &parameters)
    {
        uint8_t size = static_cast<uint8_t>(parameters.size());
        PutParameter(buffer, pos, size);
        for (const auto &p : parameters)
        {
            size = static_cast<uint8_t>(p.first.size());
            PutParameter(buffer, pos, size);

            std::memcpy(buffer + pos, p.first.data(), size);
            pos += size;

            size = static_cast<uint8_t>(p.second.size());
            PutParameter(buffer, pos, size);

            std::memcpy(buffer + pos, p.second.data(), size);
            pos += size;
        }
    }

    template <typename U>
    Params GetParameters(const char *buffer, U &pos)
    {
        Params ret;
        uint8_t params = GetParameter<uint8_t>(buffer, pos);
        for (uint8_t i = 0; i < params; ++i)
        {
            uint8_t size = GetParameter<uint8_t>(buffer, pos);
            std::string key =
                std::string(reinterpret_cast<const char *>(buffer + pos), size);
            pos += size;
            size = GetParameter<uint8_t>(buffer, pos);
            std::string value =
                std::string(reinterpret_cast<const char *>(buffer + pos), size);
            pos += size;
            ret[key] = value;
        }
        return ret;
    }

private:
    void CheckCallbackType(const std::string type) const;
};

} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_CORE_OPERATOR_H_ */
