/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataManCallbackReceiver.cpp
 *
 *  Created on: Jan 9, 2017
 *      Author: Jason Wang
 */

#include <chrono>
#include <iostream>
#include <mpi.h>
#include <numeric>
#include <thread>
#include <vector>

#include "adios2/common/ADIOSMacros.h"
#include <adios2.h>

// matches Signature2 in ADIOS2
void UserCallBack(void *data, const std::string &doid, const std::string &var,
                  const std::string &type, const size_t step,
                  const adios2::Dims &shape, const adios2::Dims &start,
                  const adios2::Dims &count)
{

    std::cout << "Object : " << doid << std::endl;
    std::cout << "Variable :" << var << std::endl;
    std::cout << "Type : " << type << std::endl;
    std::cout << "Shape : [";
    for (size_t i = 0; i < count.size(); ++i)
    {
        std::cout << count[i];
        if (i != count.size() - 1)
        {
            std::cout << ", ";
        }
    }
    std::cout << "]" << std::endl;

    size_t varsize = std::accumulate(count.begin(), count.end(), 1,
                                     std::multiplies<std::size_t>());

    size_t dumpsize = 128;
    if (varsize < dumpsize)
    {
        dumpsize = varsize;
    }
    dumpsize = varsize;

    std::cout << "Data : " << std::endl;

#define declare_type(T)                                                        \
    if (type == adios2::GetType<T>())                                          \
    {                                                                          \
        for (size_t i = 0; i < dumpsize; ++i)                                  \
        {                                                                      \
            std::cout << (reinterpret_cast<T *>(data))[i] << " ";              \
        }                                                                      \
        std::cout << std::endl;                                                \
    }
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
}

int main(int argc, char *argv[])
{

    MPI_Init(&argc, &argv);
    int timeout = 20;

    if (argc == 2)
    {
        timeout = atoi(argv[1]);
    }

    try
    {
        adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);

        adios2::Operator callbackFloat = adios.DefineOperator(
            "Print float Variable callback",
            std::function<void(void *, const std::string &, const std::string &,
                               const std::string &, const size_t,
                               const adios2::Dims &, const adios2::Dims &,
                               const adios2::Dims &)>(UserCallBack));

        adios2::IO dataManIO = adios.DeclareIO("WAN");
        dataManIO.SetEngine("DataMan");
        dataManIO.SetParameters({
            {"WorkflowMode", "subscribe"},
        });
        dataManIO.AddTransport("WAN", {
                                          {"Library", "ZMQ"},
                                          {"IPAddress", "127.0.0.1"},
                                          {"Port", "12306"},
                                      });
        dataManIO.AddOperation(callbackFloat); // propagate to all Engines

        adios2::Engine dataManReader =
            dataManIO.Open("stream", adios2::Mode::Read);

        std::this_thread::sleep_for(std::chrono::seconds(timeout));

        dataManReader.Close();
    }
    catch (std::invalid_argument &e)
    {
        std::cout << "Invalid argument exception, STOPPING PROGRAM"
                  << "\n";
        std::cout << e.what() << "\n";
    }
    catch (std::ios_base::failure &e)
    {
        std::cout << "IO System base failure exception, STOPPING PROGRAM "
                  << "\n";
        std::cout << e.what() << "\n";
    }
    catch (std::exception &e)
    {
        std::cout << "Exception, STOPPING PROGRAM"
                  << "\n";
        std::cout << e.what() << "\n";
    }

    return 0;
}
