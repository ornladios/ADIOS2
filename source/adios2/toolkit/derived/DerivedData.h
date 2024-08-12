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
}
}
#endif
