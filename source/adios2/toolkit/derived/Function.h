#ifndef ADIOS2_DERIVED_Function_H_
#define ADIOS2_DERIVED_Function_H_

#include "DerivedData.h"

namespace adios2
{
namespace derived
{
DerivedData AddFunc(ExprData input);
DerivedData SubtractFunc(ExprData input);
DerivedData SinFunc(ExprData input);
DerivedData CosFunc(ExprData input);
DerivedData TanFunc(ExprData input);
DerivedData AsinFunc(ExprData input);
DerivedData AcosFunc(ExprData input);
DerivedData AtanFunc(ExprData input);
DerivedData MultFunc(ExprData input);
DerivedData DivFunc(ExprData input);
DerivedData SqrtFunc(ExprData input);
DerivedData PowFunc(ExprData input);
DerivedData MagnitudeFunc(ExprData input);
DerivedData Cross3DFunc(ExprData input);
DerivedData Curl3DFunc(ExprData input);

std::tuple<Dims, Dims, Dims> SameDimsFunc(std::vector<std::tuple<Dims, Dims, Dims>> input,
                                          bool constants);
std::tuple<Dims, Dims, Dims> SameDimsWithAgrFunc(std::vector<std::tuple<Dims, Dims, Dims>> input,
                                                 bool constants);
std::tuple<Dims, Dims, Dims> Cross3DDimsFunc(std::vector<std::tuple<Dims, Dims, Dims>> input,
                                             bool constants);
std::tuple<Dims, Dims, Dims> CurlDimsFunc(std::vector<std::tuple<Dims, Dims, Dims>> input,
                                          bool constants);

DataType SameTypeFunc(DataType input);
DataType FloatTypeFunc(DataType input);
}
}
#endif
