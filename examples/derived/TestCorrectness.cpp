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
  size_t rank, size;
    const size_t Nx = 2, Ny = 2, Nz = 10;
    const size_t steps = 2;
    /** Application variable */
    std::default_random_engine generator;
    std::uniform_real_distribution<float> distribution(0.0, 10.0);

    std::vector<float> simArray1(Nx * Ny * Nz);
    std::vector<float> simArray2(Nx * Ny * Nz);
    std::vector<float> simArray3(Nx * Ny * Nz);
    std::vector<float> simArray4(Nx * Ny * Nz);
    for (size_t i = 0; i < Nx * Ny * Nz; ++i)
    {
        simArray1[i] = distribution(generator);
        simArray2[i] = distribution(generator);
        simArray3[i] = distribution(generator);
        simArray4[i] = distribution(generator);
    }

    adios2::ADIOS adios;

    adios2::IO bpIO = adios.DeclareIO("BPExpression");
    bpIO.SetEngine("bp5");
    bpIO.DefineAttribute<int>("nsteps", steps);
    bpIO.DefineAttribute<int>("arraySize", Nx * Ny * Nz);
    bpIO.SetParameters("statslevel=1");
    bpIO.SetParameters("statsblocksize=5000");

    auto Ux =
        bpIO.DefineVariable<float>("sim/Ux", {Nz, Ny, size * Nx}, {0, 0, rank * Nx}, {Nz, Ny, Nx});
    auto Uy =
        bpIO.DefineVariable<float>("sim/Uy", {Nz, Ny, size * Nx}, {0, 0, rank * Nx}, {Nz, Ny, Nx});
    auto Uz =
        bpIO.DefineVariable<float>("sim/Uz", {Nz, Ny, size * Nx}, {0, 0, rank * Nx}, {Nz, Ny, Nx});
    auto addU = bpIO.DefineDerivedVariable("derive/addU",
                                           "x:sim/Ux \n"
                                           "y:sim/Uy \n"
                                           "z:sim/Uz \n"
                                           "x+y+z",
                                           adios2::DerivedVarType::StoreData);
    // TODO add Operation to magU

    std::string filename = "expAdd.bp";
    adios2::Engine bpFileWriter = bpIO.Open(filename, adios2::Mode::Write);

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
    if (rank == 0)
    {
        std::cout << "Wrote file " << filename << " to disk. \n";
    }


    // TODO add Operation to magU

    adios2::Engine bpFileReader = bpIO.Open(filename, adios2::Mode::Read);

    std::vector<float> uxi(Nx * Ny * Nz);
    std::vector<float> uyi(Nx * Ny * Nz);
    std::vector<float> uzi(Nx * Ny * Nz);
    std::vector<float> addUi(Nx * Ny * Nz);
    float calcA;
    float epsilon = 0.000001;
    for (int i = 0; i < steps; i++)
    {
        std::cout << "Reading step " << i << std::endl;
        bpFileReader.BeginStep();
        uxi = bpFileReader.Get(Ux);
        uyi = bpFileReader.Get(Uy);
        uzi = bpFileReader.Get(Uz);
        addUi = bpFileReader.Get(addU);
	for (int x = 0; x < Nx; ++x)
	  {
	    for (int y = 0; y < Ny; ++y)
	      {
		for (int z = 0; z < Nz; ++z)
		  {
		    calcA = uxi[x,y,z] + uyi[x,y,z] + uzi[x,y,z];
		    string eq;
		    if (fabs(calcA - addUi) < epsilon)
		      {
			std::cout << "TRUE";
			eq = " = ";
		      }
		    else
		      {
			std::cout << "FALSE";
			eq = " != ";
		      }
		    std::cout << "addU " << i << " [" << x << "," << y << "," << z << "]";
		    std::cout << eq << calcA << std::endl;
		    bpFileReader.EndStep();
		  }
	      }
	  }
    }

    /** Create bp file, engine becomes unreachable after this*/
    bpFileWriter.Close();
    if (rank == 0)
    {
        std::cout << "Read file " << filename << " complete. \n";
    }
    
    
    
    return 0;
}


//TEST(DerivedTest, MagCorrectnessTest)
void MagCorrectnessTest()
{
    const size_t Nx = 2, Ny = 2, Nz = 10;
    const size_t steps = 2;
    // Application variable
    std::default_random_engine generator;
    std::uniform_real_distribution<float> distribution(0.0, 10.0);

    std::vector<float> simArray1(Nx * Ny * Nz);
    std::vector<float> simArray2(Nx * Ny * Nz);
    std::vector<float> simArray3(Nx * Ny * Nz);
    std::vector<float> simArray4(Nx * Ny * Nz);
    for (size_t i = 0; i < Nx * Ny * Nz; ++i)
    {
        simArray1[i] = distribution(generator);
        simArray2[i] = distribution(generator);
        simArray3[i] = distribution(generator);
        simArray4[i] = distribution(generator);
    }

    adios2::ADIOS adios;

    adios2::IO bpIO = adios.DeclareIO("BPExpression");
    bpIO.SetEngine("bp5");
    bpIO.DefineAttribute<int>("nsteps", steps);
    bpIO.DefineAttribute<int>("arraySize", Nx * Ny * Nz);
    bpIO.SetParameters("statslevel=1");
    bpIO.SetParameters("statsblocksize=5000");

    auto Ux =
        bpIO.DefineVariable<float>("sim/Ux", {Nz, Ny, size * Nx}, {0, 0, rank * Nx}, {Nz, Ny, Nx});
    auto Uy =
        bpIO.DefineVariable<float>("sim/Uy", {Nz, Ny, size * Nx}, {0, 0, rank * Nx}, {Nz, Ny, Nx});
    auto Uz =
        bpIO.DefineVariable<float>("sim/Uz", {Nz, Ny, size * Nx}, {0, 0, rank * Nx}, {Nz, Ny, Nx});
    auto magU = bpIO.DefineDerivedVariable("derive/magU",
                                           "x:sim/Ux \n"
                                           "y:sim/Uy \n"
                                           "z:sim/Uz \n"
                                           "magnitude(x,y,z)",
					   adios2::DerivedVarType::StoreData);
    // TODO add Operation to magU

    std::string filename = "expMagnitude.bp";
    adios2::Engine bpFileWriter = bpIO.Open(filename, adios2::Mode::Write);

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
    if (rank == 0)
    {
        std::cout << "Wrote file " << filename << " to disk. \n";
    }

    // TODO add Operation to magU

    adios2::Engine bpFileReader = bpIO.Open(filename, adios2::Mode::Read);

    std::vector<float> uxi(Nx * Ny * Nz);
    std::vector<float> uyi(Nx * Ny * Nz);
    std::vector<float> uzi(Nx * Ny * Nz);
    std::vector<float> magUi(Nx * Ny * Nz);
    float calcM;
    float epsilon = 0.000001;
    for (int i = 0; i < steps; i++)
    {
        std::cout << "Reading step " << i << std::endl;
        bpFileReader.BeginStep();
        uxi = bpFileReader.Get(Ux);
        uyi = bpFileReader.Get(Uy);
        uzi = bpFileReader.Get(Uz);
        magUi = bpFileReader.Get(magU);
	for (int x = 0; x < Nx; ++x)
	  {
	    for (int y = 0; y < Ny; ++y)
	      {
		for (int z = 0; z < Nz; ++z)
		  {
		    calcM = sqrt(pow(uxi[x,y,z],2)+pow(uyi[x,y,z],2)+pow(uzi[x,y,z],2));
		    string eq;
		    if (fabs(calcM - magUi) < epsilon)
		      {
			std::cout << "TRUE";
			eq = " = ";
		      }
		    else
		      {
			std::cout << "FALSE";
			eq = " != ";
		      }
		    std::cout << "magU " << i << " [" << x << "," << y << "," << z << "]";
		    std::cout << eq << calcM << std::endl;
		    bpFileReader.EndStep();
		  }
	      }
	  }
    }

    // Create bp file, engine becomes unreachable after this
    bpFileWriter.Close();
    if (rank == 0)
    {
        std::cout << "Read file " << filename << " complete. \n";
    }
    
    
    
    return 0;
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
