/* MyArrayAdd
 * Adds first input (a double value) to each element of second input (an double array)
 * c = MyArrayAdd(a,b);
*/

#include "mex.hpp"
#include "mexAdapter.hpp"

using namespace matlab::data;
using matlab::mex::ArgumentList;
using matlab::engine::convertUTF8StringToUTF16String;

class MexFunction : public matlab::mex::Function {
public:
    void operator()(ArgumentList outputs, ArgumentList inputs) {
        checkArguments(outputs, inputs);        
        const double offSet = inputs[0][0];
        TypedArray<double> doubleArray = std::move(inputs[1]);
        for (auto& elem : doubleArray) {
            elem += offSet;
        }
        outputs[0] = doubleArray;
    }

    void checkArguments(ArgumentList outputs, ArgumentList inputs) {
        // Get pointer to engine
        std::shared_ptr<matlab::engine::MATLABEngine> matlabPtr = getEngine();

        // Get array factory
        ArrayFactory factory;

        // Check first input argument
        if (inputs[0].getType() != ArrayType::DOUBLE ||
            inputs[0].getType() == ArrayType::COMPLEX_DOUBLE ||
            inputs[0].getNumberOfElements() != 1)
        {
            matlabPtr->feval(convertUTF8StringToUTF16String("error"),
                0,
                std::vector<Array>({ factory.createScalar("First input must scalar double") }));
        }

        // Check second input argument
        if (inputs[1].getType() != ArrayType::DOUBLE ||
            inputs[1].getType() == ArrayType::COMPLEX_DOUBLE)
        {
            matlabPtr->feval(convertUTF8StringToUTF16String("error"),
                0,
                std::vector<Array>({ factory.createScalar("Input must double array") }));
        }
        // Check number of outputs
        if (outputs.size() > 1) {
            matlabPtr->feval(convertUTF8StringToUTF16String("error"),
                0,
                std::vector<Array>({ factory.createScalar("Only one output is returned") }));
        }
    }
};
