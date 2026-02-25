/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * PluginOperatorInterface.h Operators using the plugin interface should derive
 * from this class.
 *
 * This header is self-contained: it depends only on ADIOSBaseTypes.h and
 * standard C++ headers. External plugin operators need only include this
 * file and do NOT need to link against adios2_core.
 *
 *  Created on: Dec 7, 2021
 *      Author: Caitlin Ross <caitlin.ross@kitware.com>
 */

#ifndef ADIOS2_OPERATOR_PLUGIN_PLUGINOPERATORINTERFACE_H_
#define ADIOS2_OPERATOR_PLUGIN_PLUGINOPERATORINTERFACE_H_

/* Plugin operator interface version.
 * v1: derived from core::Operator (ADIOS2 <= 2.11)
 * v2: standalone, self-contained interface (ADIOS2 >= 2.12)
 */
#define ADIOS2_PLUGIN_OPERATOR_INTERFACE 2

#include "adios2/common/ADIOSBaseTypes.h"

#include <cstdint>
#include <cstring>

namespace adios2
{
namespace plugin
{

/** An operator interface to be used by the plugin infrastructure.
 *
 * External plugin operators should inherit from this class and implement
 * at minimum: Operate(), InverseOperate(), and IsDataTypeValid().
 *
 * The plugin shared library must export two C functions:
 *   extern "C" {
 *       PluginOperatorInterface *OperatorCreate(const Params &parameters);
 *       void OperatorDestroy(PluginOperatorInterface *obj);
 *   }
 */
class PluginOperatorInterface
{
public:
    PluginOperatorInterface(const Params &parameters) : m_Parameters(parameters) {}
    virtual ~PluginOperatorInterface() = default;

    /**
     * @param dataIn input data buffer
     * @param blockStart starting offsets of the block
     * @param blockCount sizes of the block in each dimension
     * @param type data type of the elements
     * @param bufferOut output buffer for transformed data
     * @return size of data written to bufferOut
     */
    virtual size_t Operate(const char *dataIn, const Dims &blockStart, const Dims &blockCount,
                           const DataType type, char *bufferOut) = 0;

    /**
     * @param bufferIn input buffer of transformed data
     * @param sizeIn size of bufferIn in bytes
     * @param dataOut output buffer for restored data
     * @return size of data written to dataOut
     */
    virtual size_t InverseOperate(const char *bufferIn, const size_t sizeIn, char *dataOut) = 0;

    /**
     * @param type data type to check
     * @return true if this operator supports the given data type
     */
    virtual bool IsDataTypeValid(const DataType type) const = 0;

    /** Give an upper bound estimate how big the transformed data could be */
    virtual size_t GetEstimatedSize(const size_t ElemCount, const size_t ElemSize,
                                    const size_t ndims, const size_t *dims) const
    {
        return ElemCount * ElemSize + 128;
    }

    /** Called to provide per-invocation parameters */
    virtual void AddExtraParameters(const Params &params) {}

    void SetParameter(const std::string key, const std::string value) noexcept
    {
        m_Parameters[key] = value;
    }

    Params &GetParameters() noexcept { return m_Parameters; }

    void SetAccuracy(const Accuracy &a) noexcept { m_AccuracyRequested = a; }
    Accuracy GetAccuracy() const noexcept { return m_AccuracyProvided; }

protected:
    /** Parameters associated with this operator */
    Params m_Parameters;

    /** user requested accuracy */
    Accuracy m_AccuracyRequested = {0.0, 0.0, false};
    /** provided accuracy */
    Accuracy m_AccuracyProvided = {0.0, 0.0, false};

    /**
     * Write a value into a buffer at the given position and advance pos.
     */
    template <typename T, typename U>
    void PutParameter(char *buffer, U &pos, const T &parameter)
    {
        std::memcpy(buffer + pos, &parameter, sizeof(T));
        pos += sizeof(T);
    }

    /**
     * Read a value from a buffer at the given position and advance pos.
     */
    template <typename T, typename U>
    T GetParameter(const char *buffer, U &pos)
    {
        T ret;
        std::memcpy(&ret, buffer + pos, sizeof(T));
        pos += sizeof(T);
        return ret;
    }
};

} // end namespace plugin
} // end namespace adios2

#endif /* ADIOS2_OPERATOR_PLUGIN_PLUGINOPERATORINTERFACE_H_ */
