/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FileStreamingRead.cpp : Reads from a stream of bp self-describing variables,
 * extract variables of unknown and known type, flush to disk
 *
 *  Created on: Sep 25, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include <adios2.h>
#include <mpi.h>

#include <vector>

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /** Application variable */
    const std::size_t Nx = 10;
    std::vector<float> myFloats;
    int *myInts = nullptr;
    void *myAnyType = nullptr;

    try
    {
        /** ADIOS class factory of IO class objects, DebugON is recommended */
        adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);

        /*** IO class object: settings and factory of Settings: Variables,
         * Parameters, Transports, and Execution: Engines */
        adios2::IO &streamingIO = adios.DeclareIO("ReadBP");
        streamingIO.SetEngine("DataManReader");
        streamingIO.SetParameters({{"MaxBufferMemory", "1Gb"}});

        auto wanStream =
            streamingIO.AddTransport("WAN", {{"IPAddress", "127.0.0.1"},
                                             {"Port", "22"},
                                             {"MaxWaitingTime", "1hr"}});
        auto fileStream = streamingIO.AddTransport(
            "File", {{"Library", "POSIX"}, {"Name", "TelescopeImages1"}});

        // assumes we know what variables we are receiving

        /**
         * Engine starts listening
         */
        adios2::Engine &bpStreamReader =
            streamingIO.Open("myVector.bp", adios2::Mode::Read);

        while (bpStreamReader.GetAdvanceStatus() == adios2::StepStatus::OK)
        {
            // Gets data until buffer is full (1Gb from SetParameters)
            bpStreamReader.Get(wanStream);

            /// EXTRACT data duplicating memory
            adios2::Variable<float> *bpFloats =
                bpStreamReader.InquireVariable<float>("bpFloats");

            // pre-allocate, this will duplicate memory (just like file Write)
            if (bpFloats != nullptr)
            {
                for (unsigned int t = 0; t < bpFloats->GetAvailableStepsCount();
                     ++t)
                {
                    myFloats.reserve(bpFloats->TotalSize());
                    myFloats.resize(0);

                    adios2::Box<std::size_t> stepBox(
                        {bpFloats->GetAvailableStepsStart() + t}, {1});
                    bpFloats->SetStepSelection(stepBox);
                    bpStreamReader.Read<float>(*bpFloats, myFloats.data());
                }
            }

            adios2::Variable<int> *bpInts =
                bpStreamReader.InquireVariable<int>("bpInts");

            /// EXTRACT data without duplicating memory
            // this will not duplicate memory, internal reference to current bp
            // buffer
            if (bpFloats != nullptr)
            {
                bpStreamReader.Read<int>(*bpInts, myInts);
            }

            /// EXTRACT any type data (this demotes to void*), without
            /// duplicating memory
            adios2::VariableBase *bpAny =
                bpStreamReader.InquireVariableAny("bpUnknown");

            if (bpAny != nullptr)
            {
                // by now we must know the type
                if (bpAny->m_Type == "int")
                {
                    bpStreamReader.Read<int>(
                        *dynamic_cast<adios2::Variable<int> *>(bpAny),
                        reinterpret_cast<int *>(myAnyType));
                }
            }

            bpStreamReader.Flush(fileStream);
        }

        bpStreamReader.Close(wanStream); // release wanStream

        // do more tasks with data in-memory...

        /** Close all opened transports (in this case only fileStream), engine
         * becomes unreachable after
         * this call */
        bpStreamReader.Close();
    }
    catch (std::invalid_argument &e)
    {
        std::cout << "Invalid argument exception, STOPPING PROGRAM from rank "
                  << rank << "\n";
        std::cout << e.what() << "\n";
    }
    catch (std::ios_base::failure &e)
    {
        std::cout << "IO System base failure exception, STOPPING PROGRAM "
                     "from rank "
                  << rank << "\n";
        std::cout << e.what() << "\n";
    }
    catch (std::exception &e)
    {
        std::cout << "Exception, STOPPING PROGRAM from rank " << rank << "\n";
        std::cout << e.what() << "\n";
    }

    MPI_Finalize();
    return 0;
}
