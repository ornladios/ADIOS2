#ifndef ADIOS2_DERIVED_Function_H_
#define ADIOS2_DERIVED_Function_H_

#include "ExprHelper.h"
#include "adios2/common/ADIOSTypes.h"
#include "adios2/helper/adiosLog.h"
#include <functional>

namespace adios2
{
namespace derived
{

struct DerivedData
{
    void *Data;
    Dims Start;
    Dims Count;
};

struct OperatorFunctions
{
    std::function<DerivedData(std::vector<DerivedData>, DataType)> ComputeFct;
    std::function<Dims(std::vector<Dims>)> DimsFct;
};

DerivedData AddFunc(std::vector<DerivedData> input, DataType type);
DerivedData MagnitudeFunc(std::vector<DerivedData> input, DataType type);
DerivedData Curl3DFunc(std::vector<DerivedData> input, DataType type);

Dims SameDimsFunc(std::vector<Dims> input);
Dims CurlDimsFunc(std::vector<Dims> input);

extern std::map<adios2::detail::ExpressionOperator, OperatorFunctions> OpFunctions;

template <class T>
T *ApplyOneToOne(std::vector<DerivedData> inputData, size_t dataSize,
                 std::function<T(T, T)> compFct);

template <class T>
T *ApplyCurl(const T *input1, const T *input2, const T *input3, const size_t dims[3]);

}
}
#endif
