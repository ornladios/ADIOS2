#ifndef ADIOS2_DERIVED_Function_H_
#define ADIOS2_DERIVED_Function_H_

#include "DerivedData.h"

namespace adios2
{
namespace derived
{
DerivedData AddFunc(std::vector<DerivedData> input, DataType type, int nproc);
DerivedData SubtractFunc(std::vector<DerivedData> input, DataType type, int nproc);
DerivedData SinFunc(std::vector<DerivedData> input, DataType type, int nproc);
DerivedData CosFunc(std::vector<DerivedData> input, DataType type, int nproc);
DerivedData TanFunc(std::vector<DerivedData> input, DataType type, int nproc);
DerivedData AsinFunc(std::vector<DerivedData> input, DataType type, int nproc);
DerivedData AcosFunc(std::vector<DerivedData> input, DataType type, int nproc);
DerivedData AtanFunc(std::vector<DerivedData> input, DataType type, int nproc);
DerivedData MultFunc(std::vector<DerivedData> input, DataType type, int nproc);
DerivedData DivFunc(std::vector<DerivedData> input, DataType type, int nproc);
DerivedData SqrtFunc(std::vector<DerivedData> input, DataType type, int nproc);
DerivedData PowFunc(std::vector<DerivedData> input, DataType type, int nproc);
DerivedData MagnitudeFunc(std::vector<DerivedData> input, DataType type, int nproc);
DerivedData Cross3DFunc(std::vector<DerivedData> input, DataType type, int nproc);
DerivedData Curl3DFunc(std::vector<DerivedData> input, DataType type, int nproc);
DerivedData MinFunc(std::vector<DerivedData> input, DataType type, int nproc);
DerivedData MaxFunc(std::vector<DerivedData> input, DataType type, int nproc);
DerivedData SumFunc(std::vector<DerivedData> input, DataType type, int nproc);
DerivedData MeanFunc(std::vector<DerivedData> input, DataType type, int nproc);
DerivedData MedianFunc(std::vector<DerivedData> input, DataType type, int nproc);
DerivedData VarianceFunc(std::vector<DerivedData> input, DataType type, int nproc);
DerivedData StDevFunc(std::vector<DerivedData> input, DataType type, int nproc);

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
