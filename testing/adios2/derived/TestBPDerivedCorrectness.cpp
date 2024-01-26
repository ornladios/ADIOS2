#include <cstdint>
#include <cstring>

#include <cmath>
#include <limits>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <vector>

#include <adios2.h>
#include <gtest/gtest.h>

TEST(DerivedCorrectness, AddCorrectnessTest)
{
    const size_t Nx = 10, Ny = 3, Nz = 6;
    const size_t steps = 2;
    /** Application variable */
    std::default_random_engine generator;
    std::uniform_real_distribution<float> distribution(0.0, 10.0);

    std::vector<float> simArray1(Nx * Ny * Nz);
    std::vector<float> simArray2(Nx * Ny * Nz);
    std::vector<float> simArray3(Nx * Ny * Nz);
    for (size_t i = 0; i < Nx * Ny * Nz; ++i)
    {
        simArray1[i] = distribution(generator);
        simArray2[i] = distribution(generator);
        simArray3[i] = distribution(generator);
    }

    adios2::ADIOS adios;

    adios2::IO bpOut = adios.DeclareIO("BPWriteAddExpression");

    std::vector<std::string> varname = {"sim1/Ux", "sim1/Uy", "sim1/Uz"};
    std::string derivedname = "derived/addU";

    auto Ux = bpOut.DefineVariable<float>(varname[0], {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    auto Uy = bpOut.DefineVariable<float>(varname[1], {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    auto Uz = bpOut.DefineVariable<float>(varname[2], {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    // clang-format off
    auto addU = bpOut.DefineDerivedVariable(derivedname,
                                            "x:" + varname[0] + " \n"
                                            "y:" + varname[1] + " \n"
                                            "z:" + varname[2] + " \n"
                                            "x+y+z",
                                            adios2::DerivedVarType::StoreData);
    // clang-format on
    std::string filename = "expAdd.bp";
    adios2::Engine bpFileWriter = bpOut.Open(filename, adios2::Mode::Write);

    for (int i = 0; i < steps; i++)
    {
        bpFileWriter.BeginStep();
        bpFileWriter.Put(Ux, simArray1.data());
        bpFileWriter.Put(Uy, simArray2.data());
        bpFileWriter.Put(Uz, simArray3.data());
        bpFileWriter.EndStep();
    }
    bpFileWriter.Close();

    adios2::IO bpIn = adios.DeclareIO("BPReadExpression");
    adios2::Engine bpFileReader = bpIn.Open(filename, adios2::Mode::Read);

    std::vector<float> readUx;
    std::vector<float> readUy;
    std::vector<float> readUz;
    std::vector<float> readAdd;

    float calcA;
    float epsilon = 0.01;
    for (int i = 0; i < steps; i++)
    {
        bpFileReader.BeginStep();
        bpFileReader.Get(varname[0], readUx);
        bpFileReader.Get(varname[1], readUy);
        bpFileReader.Get(varname[2], readUz);
        bpFileReader.Get(derivedname, readAdd);
        bpFileReader.EndStep();

        for (size_t ind = 0; ind < Nx * Ny * Nz; ++ind)
        {
            calcA = readUx[ind] + readUy[ind] + readUz[ind];
            EXPECT_TRUE(fabs(calcA - readAdd[ind]) < epsilon);
        }
    }
    bpFileReader.Close();
}

TEST(DerivedCorrectness, MagCorrectnessTest)
{
    const size_t Nx = 2, Ny = 3, Nz = 10;
    const size_t steps = 2;
    // Application variable
    std::default_random_engine generator;
    std::uniform_real_distribution<float> distribution(0.0, 10.0);

    std::vector<float> simArray1(Nx * Ny * Nz);
    std::vector<float> simArray2(Nx * Ny * Nz);
    std::vector<float> simArray3(Nx * Ny * Nz);
    for (size_t i = 0; i < Nx * Ny * Nz; ++i)
    {
        simArray1[i] = distribution(generator);
        simArray2[i] = distribution(generator);
        simArray3[i] = distribution(generator);
    }

    adios2::ADIOS adios;
    adios2::IO bpOut = adios.DeclareIO("BPWriteExpression");
    std::vector<std::string> varname = {"sim2/Ux", "sim2/Uy", "sim2/Uz"};
    std::string derivedname = "derived/magU";

    auto Ux = bpOut.DefineVariable<float>(varname[0], {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    auto Uy = bpOut.DefineVariable<float>(varname[1], {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    auto Uz = bpOut.DefineVariable<float>(varname[2], {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    // clang-format off
    auto magU = bpOut.DefineDerivedVariable(derivedname,
                                            "x:" + varname[0] + " \n"
                                            "y:" + varname[1] + " \n"
                                            "z:" + varname[2] + " \n"
                                            "magnitude(x,y,z)",
                                            adios2::DerivedVarType::StoreData);
    // clang-format on
    std::string filename = "expMagnitude.bp";
    adios2::Engine bpFileWriter = bpOut.Open(filename, adios2::Mode::Write);

    for (int i = 0; i < steps; i++)
    {
        bpFileWriter.BeginStep();
        bpFileWriter.Put(Ux, simArray1.data());
        bpFileWriter.Put(Uy, simArray2.data());
        bpFileWriter.Put(Uz, simArray3.data());
        bpFileWriter.EndStep();
    }
    bpFileWriter.Close();

    adios2::IO bpIn = adios.DeclareIO("BPReadMagExpression");
    adios2::Engine bpFileReader = bpIn.Open(filename, adios2::Mode::Read);

    std::vector<float> readUx;
    std::vector<float> readUy;
    std::vector<float> readUz;
    std::vector<float> readMag;

    float calcM;
    float epsilon = 0.01;
    for (int i = 0; i < steps; i++)
    {
        bpFileReader.BeginStep();
        auto varx = bpIn.InquireVariable<float>(varname[0]);
        auto vary = bpIn.InquireVariable<float>(varname[1]);
        auto varz = bpIn.InquireVariable<float>(varname[2]);
        auto varmag = bpIn.InquireVariable<float>(derivedname);

        bpFileReader.Get(varx, readUx);
        bpFileReader.Get(vary, readUy);
        bpFileReader.Get(varz, readUz);
        bpFileReader.Get(varmag, readMag);
        bpFileReader.EndStep();

        for (size_t ind = 0; ind < Nx * Ny * Nz; ++ind)
        {
            calcM = sqrt(pow(readUx[ind], 2) + pow(readUy[ind], 2) + pow(readUz[ind], 2));
            EXPECT_TRUE(fabs(calcM - readMag[ind]) < epsilon);
        }
    }
    bpFileReader.Close();
}

/*
 * Linear Interpolation - average difference around point "idx"
 *
 */
// T linear_interp (T* data, size_t idx, size_t count, size_t stride, size_t margin, bool center)
float linear_interp (std::vector<float> data, size_t idx, size_t count, size_t stride)
{
    size_t ind1 = idx - stride;
    size_t ind2 = idx + stride;
    bool boundary = false;
    if (idx < stride)
      {
	ind1 = idx;
	boundary = true;
      }
    if (count - idx <= stride)
      {
	ind2 = idx;
	boundary = true;
      }
     // If stride is out of bounds in both directions, ind1 = ind2 = idx
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
//std::vector<T*> computecurl3D
//const std::vector<DerivedData> inputData, size_t xcount, size_t ycount, size_t zcount, size_t margin, bool center, std::function<T(T*, size_t, size_t, size_t, size_t, bool)> pdcomp)
std::vector<std::vector<float>> computecurl3D (std::vector<float> inputX, std::vector<float> inputY, std::vector<float> inputZ, size_t xcount, size_t ycount, size_t zcount)
{
    // ToDo - verify how to navigate over the inputData spaces
    size_t dataSize = xcount * ycount * zcount;
    size_t xstride = ycount * zcount;
    size_t ystride = zcount;
    size_t zstride = 1;

    std::vector<float> curlx(dataSize);
    std::vector<float> curly(dataSize);
    std::vector<float> curlz(dataSize);
    std::vector<std::vector<float>> curl = {curlx, curly, curlz};
    // std::vector<T*> curl = {(T*)malloc(xcount * ycount * zcount * sizeof(T)), (T*)malloc(xcount * ycount * zcount * sizeof(T)), (T*)malloc(xcount * ycount * zcount * sizeof(T))};

    for (size_t i = 0; i < xcount; ++i)
    {
        for (size_t j = 0; j < ycount; ++j)
        {
            for (size_t k = 0; k < zcount; ++k)
            {
                size_t idx = (i * xstride) + (j * ystride) + (k * zstride);
                curl[0][idx] = linear_interp(inputZ, idx, dataSize, ystride) - linear_interp(inputY, idx, dataSize, zstride);
                curl[1][idx] = linear_interp(inputX, idx, dataSize, zstride) - linear_interp(inputZ, idx, dataSize, xstride);
                curl[2][idx] = linear_interp(inputY, idx, dataSize, xstride) - linear_interp(inputX, idx, dataSize, ystride);
            }
        }
    }

    return curl;
}

TEST(DerivedCorrectness, CurlCorrectnessTest)
{
    const size_t Nx = 5, Ny = 7, Nz = 11;

    // Application variable
    std::vector<float> simArray1(Nx * Ny * Nz);
    std::vector<float> simArray2(Nx * Ny * Nz);
    std::vector<float> simArray3(Nx * Ny * Nz);
    for (float x = 0; x < Nx; ++x)
    {
        for (float y = 0; y < Ny; ++y)
        {
	    for (float z = 0; z < Nz; ++z)
	    {
	      size_t idx = (x * Ny * Nz) + (y * Nz) + z;
	      /**/// Linear curl example
	      simArray1[idx] = (6 * x * y) + (7 * z);
	      simArray2[idx] = (4 * x * z) + pow(y, 2);
	      simArray3[idx] = sqrt(z) + (2 * x * y);
	      /* */
	      /*/// Less linear example
	      simArray1[idx] = sin(z);
	      simArray2[idx] = 4 * x;
	      simArray3[idx] = pow(y, 2) * cos(x);
	      */
	      /*/// Nonlinear example
	      simArray1[idx] = exp(2 * y) * sin(x);
	      simArray2[idx] = sqrt(z) * cos(x);
	      simArray3[idx] = pow(x, 2) * sin(y) + (6 * z);
	      */
	    }
        }
    }

    adios2::ADIOS adios;
    adios2::IO bpOut = adios.DeclareIO("BPWriteExpression");
    std::vector<std::string> varname = {"sim3/VX", "sim3/VY", "sim3/VZ"};
    std::string derivedname = "derived/curlV";

    auto VX = bpOut.DefineVariable<float>(varname[0], {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    auto VY = bpOut.DefineVariable<float>(varname[1], {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    auto VZ = bpOut.DefineVariable<float>(varname[2], {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    // clang-format off
    auto curlV = bpOut.DefineDerivedVariable(derivedname,
                                            "Vx:" + varname[0] + " \n"
                                            "Vy:" + varname[1] + " \n"
                                            "Vz:" + varname[2] + " \n"
                                            "curl(Vx,Vy,Vz)",
                                            adios2::DerivedVarType::StoreData);
    // clang-format on
    std::string filename = "expCurl.bp";
    adios2::Engine bpFileWriter = bpOut.Open(filename, adios2::Mode::Write);

    bpFileWriter.BeginStep();
    bpFileWriter.Put(VX, simArray1.data());
    bpFileWriter.Put(VY, simArray2.data());
    bpFileWriter.Put(VZ, simArray3.data());
    bpFileWriter.EndStep();
    bpFileWriter.Close();

    adios2::IO bpIn = adios.DeclareIO("BPReadCurlExpression");
    adios2::Engine bpFileReader = bpIn.Open(filename, adios2::Mode::Read);

    std::vector<float> readVX;
    std::vector<float> readVY;
    std::vector<float> readVZ;
    // TODO/DEBUG - VERIFY DATATYPE
    std::vector<float> readCurl;

    std::vector<std::vector<float>> calcCurl;
    double sum_x = 0;
    double sum_y = 0;
    double sum_z = 0;
    // keep count of how many data points were ignored
    // error values will be skipped for data values of infinity
    size_t inf_x = 0;
    size_t inf_y = 0;
    size_t inf_z = 0;
    bpFileReader.BeginStep();
    auto varVX = bpIn.InquireVariable<float>(varname[0]);
    auto varVY = bpIn.InquireVariable<float>(varname[1]);
    auto varVZ = bpIn.InquireVariable<float>(varname[2]);
    auto varCurl = bpIn.InquireVariable<float>(derivedname);

    bpFileReader.Get(varVX, readVX);
    bpFileReader.Get(varVY, readVY);
    bpFileReader.Get(varVZ, readVZ);
    bpFileReader.Get(varCurl, readCurl);
    bpFileReader.EndStep();

    float curl_x, curl_y, curl_z;
    float err_x, err_y, err_z;
    for (float x = 0; x < Nx; ++x)
    {
	for (float y = 0; y < Ny; ++y)
	{
	    for (float z = 0; z < Nz; ++z)
	    {
		size_t idx = (x * Ny * Nz) + (y * Nz) + z;
		/**/// Linear example
		curl_x = -(2 * x);
		curl_y = 7 - (2 * y);
		curl_z = (4 * z) - (6 * x);
		/* */
		/*/// Less linear
		curl_x = 2 * y * cos(x);
		curl_y = cos(z) + (pow(y, 2) * sin(x));
		curl_z = 4;
		*/
		/*/// Nonlinear example
		  curl_x = pow(x, 2) * cos(y) - (cos(x) / (2 * sqrt(z)));
		  curl_y = -2 * x * sin(y);
		  curl_z = -sqrt(z) * sin(x) - (2 * exp(2 * y) * sin(x));
		*/
		if (fabs(curl_x) == std::numeric_limits<float>::infinity())
		{
		    err_x = 0;
		    ++inf_x;
		}
		else
		{
		    err_x = fabs(curl_x - readCurl[3 * idx]) / (1 + fabs(curl_x));
		}
		if (fabs(curl_y) == std::numeric_limits<float>::infinity())
	        {
		    err_y = 0;
		    ++inf_y;
		}
		else
	        {
		    err_y = fabs(curl_y - readCurl[3 * idx + 1]) / (1 + fabs(curl_y));
		}
		if (fabs(curl_z) == std::numeric_limits<float>::infinity())
	        {
		    err_z = 0;
		    ++inf_z;
		}
		else
	        {
		    err_z = fabs(curl_z - readCurl[3 * idx + 2]) / (1 + fabs(curl_z));
		}
	      sum_x += err_x;
	      sum_y += err_y;
	      sum_z += err_z;
	      std::cout << "Test curl at (" << x << ", " << y << ", " << z << ")\n";
	      std::cout << "\tExpected: <" << curl_x << ", " << curl_y << ", " << curl_z << ">\n";
	      std::cout << "\t    Read: <" << readCurl[3 * idx] << ", " << readCurl[3 * idx + 1] << ", " << readCurl[3 * idx + 2] << ">" << std::endl;
	      std::cout << "\tRelative error: " << err_x << ", " << err_y << ", " << err_z << std::endl;
	    }
	}
    }
    bpFileReader.Close();
    std::cout << "Finished reading curl. Ave rel error: " << (sum_x + sum_y + sum_z)/ ((3 * Nx * Ny * Nz) - (inf_x + inf_y + inf_z)) << std::endl;
    std::cout << "\tave in x: " << (sum_x / ((Nx * Ny * Nz) - inf_x)) << std::endl;
    std::cout << "\tave in y: " << (sum_y / ((Nx * Ny * Nz) - inf_y)) << std::endl;
    std::cout << "\tave in z: " << (sum_z / ((Nx * Ny * Nz) - inf_z)) << std::endl;
    EXPECT_LT((sum_x + sum_y + sum_z) / ((3 * Nx * Ny * Nz) - (inf_x + inf_y + inf_z)), 0.5);
    EXPECT_LT(sum_x / ((Nx * Ny * Nz) - inf_x), 0.7);
    EXPECT_LT(sum_y / ((Nx * Ny * Nz) - inf_y), 0.7);
    EXPECT_LT(sum_z / ((Nx * Ny * Nz) - inf_z), 0.7);
}

int main(int argc, char **argv)
{
    int result;
    ::testing::InitGoogleTest(&argc, argv);

    result = RUN_ALL_TESTS();

    return result;
}
