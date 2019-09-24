/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BufferSTL.h
 *
 *  Created on: Sep 26, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BUFFER_HEAP_BUFFERSTL_H_
#define ADIOS2_TOOLKIT_FORMAT_BUFFER_HEAP_BUFFERSTL_H_

#include "adios2/toolkit/format/buffer/Buffer.h"

#include "adios2/common/ADIOSMacros.h"

namespace adios2
{
namespace format
{

class BufferSTL : public Buffer
{
public:
    std::vector<char> m_Buffer;

    BufferSTL();
    ~BufferSTL() = default;

    char *Data() noexcept final;
    const char *Data() const noexcept final;

    void Resize(const size_t size, const std::string hint) final;

    void Reset(const bool resetAbsolutePosition,
               const bool zeroInitialize) final;

    size_t GetAvailableSize() const final;

    template <class T>
    size_t Align() const noexcept;
};

#define declare_template_instantiation(T)                                      \
    extern template size_t BufferSTL::Align<T>() const noexcept;

ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_FORMAT_BUFFER_HEAP_BUFFERSTL_H_ */
