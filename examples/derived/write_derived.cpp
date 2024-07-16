#include <cstdint>
#include <cstring>

#include <cmath>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <vector>

#include <adios2.h>

int main(int argc, char **argv)
{
    const size_t Nn = 320;
    const size_t Nx = Nn, Ny = Nn, Nz = Nn;

    // Application variable
    std::vector<float> simArray1(Nx * Ny * Nz);
    std::vector<float> simArray2(Nx * Ny * Nz);
    std::vector<float> simArray3(Nx * Ny * Nz);
    for (size_t i = 0; i < Nx; ++i)
    {
        for (size_t j = 0; j < Ny; ++j)
        {
            for (size_t k = 0; k < Nz; ++k)
            {
                size_t idx = (i * Ny * Nz) + (j * Nz) + k;
                float x = static_cast<float>(i);
                float y = static_cast<float>(j);
                float z = static_cast<float>(k);
                // Linear curl example
                simArray1[idx] = (6 * x * y) + (7 * z);
                simArray2[idx] = (4 * x * z) + powf(y, 2);
                simArray3[idx] = sqrtf(z) + (2 * x * y);
                /* Less linear example
                simArray1[idx] = sinf(z);
                simArray2[idx] = 4 * x;
                simArray3[idx] = powf(y, 2) * cosf(x);
                */
                /* Nonlinear example
                simArray1[idx] = expf(2 * y) * sinf(x);
                simArray2[idx] = sqrtf(z + 1) * cosf(x);
                simArray3[idx] = powf(x, 2) * sinf(y) + (6 * z);
                */
            }
        }
    }

    adios2::ADIOS adios;
    adios2::IO bpOut = adios.DeclareIO("BPWriteExpression");

    auto VX = bpOut.DefineVariable<float>("sim/VX", {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    auto VY = bpOut.DefineVariable<float>("sim/VY", {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    auto VZ = bpOut.DefineVariable<float>("sim/VZ", {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz});
    // clang-format off
    //*
    bpOut.DefineDerivedVariable("derived/curlV",
                                "Vx =sim/VX \n"
                                "Vy =sim/VY \n"
                                "Vz =sim/VZ \n"
                                "curl(Vx,Vy,Vz)",
                                adios2::DerivedVarType::StoreData);
    //*/
    /*
    bpOut.DefineDerivedVariable("derived/magV",
                                "Vx =sim/VX \n"
                                "Vy =sim/VY \n"
                                "Vz =sim/VZ \n"
                                "magnitude(Vx,Vy,Vz)",
                                adios2::DerivedVarType::StoreData);
    */
    /*
    bpOut.DefineDerivedVariable("derived/addV",
                                "Vx =sim/VX \n"
                                "Vy =sim/VY \n"
                                "Vx + Vy",
                                adios2::DerivedVarType::StoreData);
    */
    /*
    bpOut.DefineDerivedVariable("derived/magofcurl",
                                "curlx =derived/curlV[::3] \n"
                                "curly =derived/curlV[1::3] \n"
                                "curlz =derived/curlV[2::3] \n"
                                "magnitude(curlx,curly,curlz)",
                                adios2::DerivedVarType::StoreData);
    */
    // clang-format on
    std::string filename = "expCurl.bp";
    adios2::Engine bpFileWriter = bpOut.Open(filename, adios2::Mode::Write);

    bpFileWriter.BeginStep();
    bpFileWriter.Put(VX, simArray1.data());
    bpFileWriter.Put(VY, simArray2.data());
    bpFileWriter.Put(VZ, simArray3.data());
    bpFileWriter.EndStep();
    bpFileWriter.Close();

    std::cout << "Example complete, check " << filename << " for data" << std::endl;

    return 0;
}
