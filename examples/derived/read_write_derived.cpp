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
    const size_t Nx = 25, Ny = 70, Nz = 13;

    // Application variables
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
            }
        }
    }

    adios2::ADIOS adios;
    adios2::IO bpOut = adios.DeclareIO("BPWriteExpression");

    adios2::IO bpIn = adios.DeclareIO("BPReadCurlExpression");
    std::string filename = "expCurl.bp";
    std::string derivedname = "derived/curlV";
    adios2::Engine bpFileReader = bpIn.Open(filename, adios2::Mode::Read);

    std::vector<float> readCurl;

    bpFileReader.BeginStep();
    auto varCurl = bpIn.InquireVariable<float>(derivedname);
    bpFileReader.Get(varCurl, readCurl);
    bpFileReader.EndStep();

    auto curlV =
        bpOut.DefineVariable<float>("copied/curlV", {Nx, Ny, Nz, 3}, {0, 0, 0, 0}, {Nx, Ny, Nz, 3});
    // clang-format off
    bpOut.DefineDerivedVariable("derived/magofcurl",
                               "curlV =copied/curlV \n"
                               "magnitude(curlV, 3)",
                               adios2::DerivedVarType::StoreData);
    // clang-format on

    adios2::Engine bpFileWriter = bpOut.Open("expMagCopiedCurl.bp", adios2::Mode::Write);

    bpFileWriter.BeginStep();
    bpFileWriter.Put(curlV, readCurl.data());
    bpFileWriter.EndStep();
    bpFileWriter.Close();

    bpFileReader.Close();

    return 0;
}
