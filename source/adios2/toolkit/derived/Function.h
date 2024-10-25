#ifndef ADIOS2_DERIVED_Function_H_
#define ADIOS2_DERIVED_Function_H_

#include "DerivedData.h"

namespace adios2
{
namespace derived
{
DerivedData AddFunc(std::vector<DerivedData> input, DataType type);
DerivedData SubtractFunc(std::vector<DerivedData> input, DataType type);
DerivedData SinFunc(std::vector<DerivedData> input, DataType type);
DerivedData CosFunc(std::vector<DerivedData> input, DataType type);
DerivedData TanFunc(std::vector<DerivedData> input, DataType type);
DerivedData AsinFunc(std::vector<DerivedData> input, DataType type);
DerivedData AcosFunc(std::vector<DerivedData> input, DataType type);
DerivedData AtanFunc(std::vector<DerivedData> input, DataType type);
DerivedData MultFunc(std::vector<DerivedData> input, DataType type);
DerivedData DivFunc(std::vector<DerivedData> input, DataType type);
DerivedData SqrtFunc(std::vector<DerivedData> input, DataType type);
DerivedData PowFunc(std::vector<DerivedData> input, DataType type);
DerivedData MagnitudeFunc(std::vector<DerivedData> input, DataType type);
DerivedData Cross3DFunc(std::vector<DerivedData> input, DataType type);
DerivedData Curl3DFunc(std::vector<DerivedData> input, DataType type);
DerivedData MinFunc(std::vector<DerivedData> input, DataType type);
DerivedData MaxFunc(std::vector<DerivedData> input, DataType type);
DerivedData SumFunc(std::vector<DerivedData> input, DataType type);
DerivedData MeanFunc(std::vector<DerivedData> input, DataType type);
DerivedData MedianFunc(std::vector<DerivedData> input, DataType type);
DerivedData VarianceFunc(std::vector<DerivedData> input, DataType type);
DerivedData StDevFunc(std::vector<DerivedData> input, DataType type);

std::tuple<Dims, Dims, Dims> SameDimsFunc(std::vector<std::tuple<Dims, Dims, Dims>> input);
std::tuple<Dims, Dims, Dims> SameDimsWithAgrFunc(std::vector<std::tuple<Dims, Dims, Dims>> input);
std::tuple<Dims, Dims, Dims> Cross3DDimsFunc(std::vector<std::tuple<Dims, Dims, Dims>> input);
std::tuple<Dims, Dims, Dims> CurlDimsFunc(std::vector<std::tuple<Dims, Dims, Dims>> input);
std::tuple<Dims, Dims, Dims> ScalarDimsFunc(std::vector<std::tuple<Dims, Dims, Dims>> input);

DataType SameTypeFunc(DataType input);
DataType FloatTypeFunc(DataType input);
}
}
#endif
