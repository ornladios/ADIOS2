
/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP4Base.inl
 *
 *  Created on: Feb 13, 2019
 *      Author: Kai Germaschewski <kai.germaschewski@unh.edu>
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP4_BP4BASE_INL_
#define ADIOS2_TOOLKIT_FORMAT_BP4_BP4BASE_INL_
#ifndef ADIOS2_TOOLKIT_FORMAT_BP4_BP4BASE_H_
#error "Inline file should only be included from its header, never on its own"
#endif

#include "BP4Base.h"

namespace adios2
{
namespace format
{

// PROTECTED

#define make_GetDataType(data_type, TYPE)                                      \
    template <>                                                                \
    inline int8_t BP4Base::GetDataType<TYPE>() const noexcept                  \
    {                                                                          \
        const int8_t type = static_cast<int8_t>(data_type);                    \
        return type;                                                           \
    }

/* clang-format off */
make_GetDataType(type_string, std::string)
make_GetDataType(type_byte, int8_t)
make_GetDataType(type_short, int16_t)
make_GetDataType(type_integer, int32_t)
make_GetDataType(type_long, int64_t)
make_GetDataType(type_unsigned_byte, uint8_t)
make_GetDataType(type_unsigned_short, uint16_t)
make_GetDataType(type_unsigned_integer, uint32_t)
make_GetDataType(type_unsigned_long, uint64_t)
make_GetDataType(type_real, float)
make_GetDataType(type_double, double)
make_GetDataType(type_long_double, long double)
make_GetDataType(type_complex, cfloat)
make_GetDataType(type_double_complex, cdouble)
/* clang-format on */

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_FORMAT_BP4_BP4BASE_INL_ */
