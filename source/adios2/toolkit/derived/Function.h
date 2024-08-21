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

std::tuple<Dims, Dims, Dims> SameDimsFunc(std::vector<std::tuple<Dims, Dims, Dims>> input);
std::tuple<Dims, Dims, Dims> CurlDimsFunc(std::vector<std::tuple<Dims, Dims, Dims>> input);

DataType SameTypeFunc(DataType input);
DataType FloatTypeFunc(DataType input);
}
}
#endif
