#include <cstdint>
#include <cstring>

#include <cmath>
#include <limits>
#include <iostream>
#include <limits>
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
    bpOut.DefineDerivedVariable(derivedname,
                                "x =" + varname[0] + " \n"
                                "y =" + varname[1] + " \n"
                                "z =" + varname[2] + " \n"
                                "x+y+z",
                                adios2::DerivedVarType::StoreData);
    // clang-format on
    std::string filename = "expAdd.bp";
    adios2::Engine bpFileWriter = bpOut.Open(filename, adios2::Mode::Write);

    for (size_t i = 0; i < steps; i++)
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
    float epsilon = (float)0.01;
    for (size_t i = 0; i < steps; i++)
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
    bpOut.DefineDerivedVariable(derivedname,
                                "x =" + varname[0] + " \n"
                                "y =" + varname[1] + " \n"
                                "z =" + varname[2] + " \n"
                                "magnitude(x,y,z)",
                                adios2::DerivedVarType::StoreData);
    // clang-format on
    std::string filename = "expMagnitude.bp";
    adios2::Engine bpFileWriter = bpOut.Open(filename, adios2::Mode::Write);

    for (size_t i = 0; i < steps; i++)
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
    float epsilon = (float)0.01;
    for (size_t i = 0; i < steps; i++)
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
            calcM = (float)sqrt(pow(readUx[ind], 2) + pow(readUy[ind], 2) + pow(readUz[ind], 2));
            EXPECT_TRUE(fabs(calcM - readMag[ind]) < epsilon);
        }
    }
    bpFileReader.Close();
}

TEST(DerivedCorrectness, CurlCorrectnessTest)
{
    const size_t Nx = 25, Ny = 70, Nz = 13;
    float error_limit = 0.0000001;

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
	      // Linear curl example
	      simArray1[idx] = (6 * x * y) + (7 * z);
	      simArray2[idx] = (4 * x * z) + pow(y, 2);
	      simArray3[idx] = sqrt(z) + (2 * x * y);
	      /* Less linear example
	      simArray1[idx] = sin(z);
	      simArray2[idx] = 4 * x;
	      simArray3[idx] = pow(y, 2) * cos(x);
	      */
	      /* Nonlinear example
	      simArray1[idx] = exp(2 * y) * sin(x);
	      simArray2[idx] = sqrt(z + 1) * cos(x);
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
                                            "Vx =" + varname[0] + " \n"
                                            "Vy =" + varname[1] + " \n"
                                            "Vz =" + varname[2] + " \n"
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
		// Linear example
		curl_x = -(2 * x);
		curl_y = 7 - (2 * y);
		curl_z = (4 * z) - (6 * x);
		/* Less linear
		curl_x = 2 * y * cos(x);
		curl_y = cos(z) + (pow(y, 2) * sin(x));
		curl_z = 4;
		*/
		/* Nonlinear example
		curl_x = pow(x, 2) * cos(y) - (cos(x) / (2 * sqrt(z + 1)));
		curl_y = -2 * x * sin(y);
		curl_z = -sqrt(z + 1) * sin(x) - (2 * exp(2 * y) * sin(x));
		*/
		if (fabs(curl_x) == std::numeric_limits<float>::infinity())
		{
		    err_x = 0;
		    ++inf_x;
		}
		else if (fabs(curl_x) < 1)
		{
		    err_x = fabs(curl_x - readCurl[3 * idx]) / (1 + fabs(curl_x));		 		  
		}
		else
		{
		    err_x = fabs(curl_x - readCurl[3 * idx]) / fabs(curl_x);
		}
		if (fabs(curl_y) == std::numeric_limits<float>::infinity())
	        {
		    err_y = 0;
		    ++inf_y;
		}
		else if (fabs(curl_y) < 1)
	        {
		    err_y = fabs(curl_y - readCurl[3 * idx + 1]) / (1 + fabs(curl_y));
		}
		else
		{
		    err_y = fabs(curl_y - readCurl[3 * idx + 1]) / fabs(curl_y);
		}
		if (fabs(curl_z) == std::numeric_limits<float>::infinity())
	        {
		    err_z = 0;
		    ++inf_z;
		}
		else if (fabs(curl_z) < 1)
	        {
		    err_z = fabs(curl_z - readCurl[3 * idx + 2]) / (1 + fabs(curl_z));
		}
		else
		{
		    err_z = fabs(curl_z - readCurl[3 * idx + 2]) / fabs(curl_z);
		}
	      sum_x += err_x;
	      sum_y += err_y;
	      sum_z += err_z;
	    }
	}
    }
    bpFileReader.Close();
    EXPECT_LT((sum_x + sum_y + sum_z) / ((3 * Nx * Ny * Nz) - (inf_x + inf_y + inf_z)), error_limit);
    EXPECT_LT(sum_x / ((Nx * Ny * Nz) - inf_x), error_limit);
    EXPECT_LT(sum_y / ((Nx * Ny * Nz) - inf_y), error_limit);
    EXPECT_LT(sum_z / ((Nx * Ny * Nz) - inf_z), error_limit);
}

int main(int argc, char **argv)
{
    int result;
    ::testing::InitGoogleTest(&argc, argv);

    result = RUN_ALL_TESTS();

    return result;
}
