/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_DERIVED_Data_H_
#define ADIOS2_DERIVED_Data_H_

#include "adios2/common/ADIOSTypes.h"

namespace adios2
{
namespace derived
{
struct DerivedData
{
    void *Data;
    Dims Start;
    Dims Count;
    DataType Type;
};

struct ExprData
{
    std::vector<DerivedData> Data;
    DataType OutType;
    void *Output = nullptr; // pre-allocated output buffer
    size_t OutputSize = 0;  // number of elements in output
};
}
}
#endif
