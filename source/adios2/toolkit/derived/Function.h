#ifndef ADIOS2_DERIVED_Function_H_
#define ADIOS2_DERIVED_Function_H_

#include "DerivedData.h"

namespace adios2
{
namespace derived
{
DerivedData AddFunc(std::vector<DerivedData> input, DataType type, bool DoCompute);
DerivedData SubtractFunc(std::vector<DerivedData> input, DataType type, bool DoCompute);
DerivedData SinFunc(std::vector<DerivedData> input, DataType type, bool DoCompute);
DerivedData CosFunc(std::vector<DerivedData> input, DataType type, bool DoCompute);
DerivedData TanFunc(std::vector<DerivedData> input, DataType type, bool DoCompute);
DerivedData AsinFunc(std::vector<DerivedData> input, DataType type, bool DoCompute);
DerivedData AcosFunc(std::vector<DerivedData> input, DataType type, bool DoCompute);
DerivedData AtanFunc(std::vector<DerivedData> input, DataType type, bool DoCompute);
DerivedData MultFunc(std::vector<DerivedData> input, DataType type, bool DoCompute);
DerivedData DivFunc(std::vector<DerivedData> input, DataType type, bool DoCompute);
DerivedData SqrtFunc(std::vector<DerivedData> input, DataType type, bool DoCompute);
DerivedData PowFunc(std::vector<DerivedData> input, DataType type, bool DoCompute);
DerivedData MagnitudeFunc(std::vector<DerivedData> input, DataType type, bool DoCompute);
DerivedData Curl3DFunc(std::vector<DerivedData> input, DataType type, bool DoCompute);

Dims SameDimsFunc(std::vector<Dims> input);
Dims CurlDimsFunc(std::vector<Dims> input);

DataType SameTypeFunc(DataType input);
DataType FloatTypeFunc(DataType input);
}
}
#endif
