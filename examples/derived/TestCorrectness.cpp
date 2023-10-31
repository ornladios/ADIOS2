#include <cstdint>
#include <cstring>

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <numeric>
#include <random>
#include <vector>

#include <adios2.h>
//#include <gtest/gtest.h>

//TEST(DerivedTest, AddCorrectnessTest)
void AddCorrectnessTest()
{
    const size_t Nx = 10, Ny = 3, Nz = 6;
    const size_t steps = 2;
    /** Application variable */
    std::default_random_engine generator;
    std::uniform_real_distribution<float> distribution(0.0, 10.0);

    std::vector<float> simArray1(Nx * Ny * Nz);
    std::vector<float> simArray2(Nx * Ny * Nz);
    std::vector<float> simArray3(Nx * Ny * Nz);
    std::cout << "Generating Data: " << std::endl;
    for (size_t i = 0; i < Nx * Ny * Nz; ++i)
    {
        simArray1[i] = distribution(generator);
        simArray2[i] = distribution(generator);
        simArray3[i] = distribution(generator);
	std::cout << "Index " << i << ": ";
	std::cout << simArray1[i] << ", ";
	std::cout << simArray2[i] << ", ";
	std::cout << simArray3[i] << std::endl;
    }

    adios2::ADIOS adios;

    adios2::IO bpOut = adios.DeclareIO("BPWriteAddExpression");

    std::vector<std::string> varname = {"sim1/Ux", "sim1/Uy", "sim1/Uz"};
    std::string derivedname = "derived/addU";

    std::cout << "Define Variable " << varname[0] << std::endl;
    auto Ux =
        bpOut.DefineVariable<float>(varname[0], {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    std::cout << "Define Variable " << varname[1] << std::endl;
    auto Uy =
        bpOut.DefineVariable<float>(varname[1], {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    std::cout << "Define Variable " << varname[2] << std::endl;
    auto Uz =
        bpOut.DefineVariable<float>(varname[2], {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    std::cout << "Define Derived Variable " << derivedname << std::endl;
    auto addU = bpOut.DefineDerivedVariable(derivedname,
                                           "x:" + varname[0] + " \n"
                                           "y:" + varname[1] + " \n"
                                           "z:" + varname[2] + " \n"
                                           "x+y+z",
                                           adios2::DerivedVarType::StoreData);
    // TODO add Operation to magU
    std::string filename = "expAdd.bp";
    adios2::Engine bpFileWriter = bpOut.Open(filename, adios2::Mode::Write);

    for (int i = 0; i < steps; i++)
    {
        std::cout << "Start step " << i << std::endl;
        bpFileWriter.BeginStep();
        bpFileWriter.Put(Ux, simArray1.data());
        bpFileWriter.Put(Uy, simArray2.data());
        bpFileWriter.Put(Uz, simArray3.data());
        bpFileWriter.EndStep();
    }

    /** Create bp file, engine becomes unreachable after this*/
    bpFileWriter.Close();
    std::cout << "Wrote file " << filename << " to disk. \n";

    // TODO add Operation to magU
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
        std::cout << "Reading step " << i << std::endl;

        bpFileReader.Get(varname[0], readUx);
        bpFileReader.Get(varname[1], readUy);
        bpFileReader.Get(varname[2], readUz);
        bpFileReader.Get(derivedname, readAdd);
	bpFileReader.EndStep();
	for (size_t ind = 0; ind < Nx*Ny*Nz; ++ind)
	  {
	    calcA = readUx[ind] + readUy[ind] + readUz[ind];
	    std::string eq;
	    if (fabs(calcA - readAdd[ind]) < epsilon)
	      {
		std::cout << "TRUE: ";
		eq = " = ";
	      }
	    else
	      {
		std::cout << "FALSE: ";
		eq = " != ";
	      }
	    std::cout << "addU " << i << " [" << ind << "] = " << readAdd[ind];
	    std::cout << eq << calcA << " = " << readUx[ind] << " + " << readUy[ind] << " + " << readUz[ind] << std::endl;
	  }
    }
    /** Create bp file, engine becomes unreachable after this*/
    bpFileReader.Close();
    std::cout << "Read file " << filename << " complete. \n";
}


//TEST(DerivedTest, MagCorrectnessTest)
void MagCorrectnessTest()
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
    
    auto Ux =
        bpOut.DefineVariable<float>(varname[0], {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    auto Uy =
        bpOut.DefineVariable<float>(varname[1], {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    auto Uz =
        bpOut.DefineVariable<float>(varname[2], {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    auto magU = bpOut.DefineDerivedVariable(derivedname,
                                           "x:" + varname[0] + " \n"
                                           "y:" + varname[1] + " \n"
                                           "z:" + varname[2] + " \n"
                                           "magnitude(x,y,z)",
					   adios2::DerivedVarType::StoreData);
    // TODO add Operation to magU
    std::string filename = "expMagnitude.bp";
    adios2::Engine bpFileWriter = bpOut.Open(filename, adios2::Mode::Write);

    for (int i = 0; i < steps; i++)
    {
        std::cout << "Start step " << i << std::endl;
        bpFileWriter.BeginStep();
        bpFileWriter.Put(Ux, simArray1.data());
        bpFileWriter.Put(Uy, simArray2.data());
        bpFileWriter.Put(Uz, simArray3.data());
        bpFileWriter.EndStep();
    }

    // Create bp file, engine becomes unreachable after this
    bpFileWriter.Close();
    std::cout << "Wrote file " << filename << " to disk. \n";

    // TODO add Operation to magU
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
        std::cout << "Reading step " << i << std::endl;
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
	for (size_t ind = 0; ind < Nx*Ny*Nz; ++ind)
	  {
	    calcM = sqrt(pow(readUx[ind],2)+pow(readUy[ind],2)+pow(readUz[ind],2));
	    std::string eq;
	    if (fabs(calcM - readMag[ind]) < epsilon)
	      {
		std::cout << "TRUE: ";
		eq = " = ";
	      }
	    else
	      {
		std::cout << "FALSE: ";
		eq = " != ";
	      }
	    std::cout << "magU " << i << " [" << ind << "]";
	    std::cout << eq << calcM << " = mag(" << readUx[ind] << ", " << readUy[ind] << ", " << readUz[ind] << ")" << std::endl;
	  }
    }
    // Create bp file, engine becomes unreachable after this
    bpFileReader.Close();
    std::cout << "Read file " << filename << " complete. \n";
}

int main(int argc, char **argv)
{
    AddCorrectnessTest();
    MagCorrectnessTest();
}

/*
int main(int argc, char **argv)
{
    int result;
    ::testing::InitGoogleTest(&argc, argv);

    if (argc > 1)
    {
        engineName = std::string(argv[1]);
    }
    result = RUN_ALL_TESTS();

    return result;
}
*/
