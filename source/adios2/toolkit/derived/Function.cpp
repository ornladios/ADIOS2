#ifndef ADIOS2_DERIVED_Function_CPP_
#define ADIOS2_DERIVED_Function_CPP_

#include "Function.h"
#include "Function.tcc"
#include "adios2/common/ADIOSMacros.h"
#include "adios2/helper/adiosFunctions.h"
#include <cmath>

namespace adios2
{
namespace derived
{

DerivedData AddFunc(std::vector<DerivedData> inputData, DataType type)
{
    size_t dataSize = std::accumulate(std::begin(inputData[0].Count), std::end(inputData[0].Count),
                                      1, std::multiplies<size_t>());

#define declare_type_add(T)                                                                        \
    if (type == helper::GetDataType<T>())                                                          \
    {                                                                                              \
        T *addValues = ApplyOneToOne<T>(inputData, dataSize, [](T a, T b) { return a + b; });      \
        return DerivedData({(void *)addValues, inputData[0].Start, inputData[0].Count});           \
    }
    ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare_type_add)
    helper::Throw<std::invalid_argument>("Derived", "Function", "AddFunc",
                                         "Invalid variable types");
    return DerivedData();
}

DerivedData MagnitudeFunc(std::vector<DerivedData> inputData, DataType type)
{
    size_t dataSize = std::accumulate(std::begin(inputData[0].Count), std::end(inputData[0].Count),
                                      1, std::multiplies<size_t>());
#define declare_type_mag(T)                                                                        \
    if (type == helper::GetDataType<T>())                                                          \
    {                                                                                              \
        T *magValues = ApplyOneToOne<T>(inputData, dataSize, [](T a, T b) { return a + b * b; });  \
        for (size_t i = 0; i < dataSize; i++)                                                      \
        {                                                                                          \
            magValues[i] = std::sqrt(magValues[i]);                                                \
        }                                                                                          \
        return DerivedData({(void *)magValues, inputData[0].Start, inputData[0].Count});           \
    }
    ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare_type_mag)
    helper::Throw<std::invalid_argument>("Derived", "Function", "MagnitudeFunc",
                                         "Invalid variable types");
    return DerivedData();
}

Dims SameDimsFunc(std::vector<Dims> input)
{
    // check that all dimenstions are the same
    if (input.size() > 1)
    {
        bool dim_are_equal = std::equal(input.begin() + 1, input.end(), input.begin());
        if (!dim_are_equal)
            helper::Throw<std::invalid_argument>("Derived", "Function", "SameDimFunc",
                                                 "Invalid variable dimensions");
    }
    // return the first dimension
    return input[0];
}

// Input Dims are the same, output is combination of all inputs
Dims AggrSameDimsFunc(std::vector<Dims> input)
{
    // check that all dimenstions are the same
    if (input.size() > 1)
    {
        bool dim_are_equal = std::equal(input.begin() + 1, input.end(), input.begin());
        if (!dim_are_equal)
            helper::Throw<std::invalid_argument>("Derived", "Function", "AggrSameDimFunc",
                                                 "Invalid variable dimensions");
    }
    // return the first dimension
    Dims output = input[0];
    size_t num_dims = output.size();
    output.insert(output.begin(), num_dims);
    return output;
}

/*
 * Linear Interpolation - average difference around point "index"
 *  can be used to approximate derivatives
 *
 * Input:
 *     data - assumed to be uniform/densely populated
 *     index - index of point of interest
 *     count - number of elements in data
 *     stride - how to access neighbours
 */
template <class T>
T linear_interp (T* data, size_t index, size_t count, size_t stride)
{
    size_t ind1 = index - stride;
    size_t ind2 = index + stride;
    bool boundary = false;
    if (index < stride)
      {
        ind1 = index;
        boundary = true;
      }
    if (count - index <= stride)
      {
        ind2 = index;
        boundary = true;
      }
     // If stride is out of bounds in both directions, ind1 = ind2 = index
     // return 0
    
     return (data[ind2] - data[ind1]) / (boundary? 1: 2);
}

/*
 * Input: 3D vector field F(x,y,z)= {F1(x,y,z), F2(x,y,z), F3(x,y,z)}
 *
 *     inputData - (3) components of 3D vector field
 *     margin - how many elements to each size will be used in approximating partial derivatives
 *     center - include point (x,y,z) in approximating of partial derivative at that point
 *
 * Computation:
 *     curl(F(x,y,z)) = (partial(F3,y) - partial(F2,z))i
 *                    + (partial(F1,z) - partial(F3,x))j
 *                    + (partial(F2,x) - partial(F1,y))k
 * 
 *     boundaries are calculated only with data in block
 *         (ex: partial derivatives in x direction at point (0,0,0)
 *              only use data from (1,0,0), etc )
 *
 * Return: 
 *     (3) components of curl
 */
/*
template <class T>
std::vector<T*> computecurl3D (const std::vector<DerivedData> inputData, size_t margin, bool center, std::function<T(T*, size_t, size_t, size_t, size_t, bool)> pdcomp)
*/
DerivedData Curl3DFunc(const std::vector<DerivedData> inputData, DataType type)
{
    // ToDo - verify how to navigate over the inputData spaces
    size_t xcount = inputData[0].Count[0];
    size_t ycount = inputData[0].Count[1];
    size_t zcount = inputData[0].Count[2];
    size_t dataSize = xcount * ycount * zcount;
    size_t xstride = ycount * zcount;
    size_t ystride = zcount;
    size_t zstride = 1;

    DerivedData curl;
    // ToDo - template type
    float* data = (float*)malloc(dataSize * sizeof(float) * 3);
    curl.Start = inputData[0].Start;
    curl.Start.insert(curl.Start.begin(), 0);
    curl.Count = inputData[0].Count;
    curl.Count.insert(curl.Count.begin(), 3);

    for (size_t i = 0; i < xcount; ++i)
    {
        for (size_t j = 0; j < ycount; ++j)
        {
            for (size_t k = 0; k < zcount; ++k)
            {
                size_t index = (i * xstride) + (j * ystride) + (k * zstride);
                data[3 * index] = linear_interp((float*)inputData[2].Data, index, dataSize, ystride) - linear_interp((float*)inputData[1].Data, index, dataSize, zstride);
                data[3 * index + 1] = linear_interp((float*)inputData[0].Data, index, dataSize, zstride) - linear_interp((float*)inputData[2].Data, index, dataSize, xstride);
                data[3 * index + 2] = linear_interp((float*)inputData[1].Data, index, dataSize, xstride) - linear_interp((float*)inputData[0].Data, index, dataSize, ystride);
            }
        }
    }

    curl.Data = data;
    return curl;
}

#define declare_template_instantiation(T)                                                          \
    T *ApplyOneToOne(std::vector<DerivedData>, size_t, std::function<T(T, T)>);

ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

}
} // namespace adios2
#endif
