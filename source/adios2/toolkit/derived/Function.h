#ifndef ADIOS2_DERIVED_Function_H_
#define ADIOS2_DERIVED_Function_H_

#include "DerivedData.h"

namespace adios2
{
namespace derived
{
DerivedData AddFunc(std::vector<DerivedData> input, DataType type);
DerivedData MagnitudeFunc(std::vector<DerivedData> input, DataType type);
DerivedData Curl3DFunc(std::vector<DerivedData> input, DataType type);

template <class T>
T linear_interp(T *data, size_t index, size_t count, size_t stride = 1);

Dims SameDimsFunc(std::vector<Dims> input);
Dims CurlDimsFunc(std::vector<Dims> input);
}
}
#endif
