#include <chrono>
#include <cstdlib>
#include <iostream>
#include <math.h>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "adios2.h"
#include "mpi.h"

#include "decomp.h"
#include "processConfig.h"
#include "settings.h"
#include "stream.h"

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    Settings settings;
    if (!settings.processArguments(argc, argv, MPI_COMM_WORLD) &&
        !settings.extraArgumentChecks())
    {
        adios2::ADIOS adios;
        if (settings.adiosConfigFileName.empty())
        {
            if (!settings.myRank && settings.verbose)
            {
                std::cout << "Use ADIOS without XML configuration "
                          << std::endl;
            }
            adios = adios2::ADIOS(settings.appComm, adios2::DebugON);
        }
        else
        {
            if (!settings.myRank && settings.verbose)
            {
                std::cout << "Use ADIOS xml file "
                          << settings.adiosConfigFileName << std::endl;
            }
            adios = adios2::ADIOS(settings.adiosConfigFileName,
                                  settings.appComm, adios2::DebugON);
        }
        Config cfg;
        size_t currentConfigLineNumber = 0;

        try
        {
            cfg = processConfig(settings, &currentConfigLineNumber);
        }
        catch (std::invalid_argument &e) // config file processing errors
        {
            if (!settings.myRank)
            {
                if (!currentConfigLineNumber)
                {
                    std::cout << "Config file error: " << e.what() << std::endl;
                }
                else
                {
                    std::cout << "Config file error in line "
                              << currentConfigLineNumber << ": " << e.what()
                              << std::endl;
                }
            }
            MPI_Finalize();
            return 0;
        }

        try
        {
            /* writing to one stream using two groups is not supported.
             * FIXME: we need to check for this condition and raise error
             */
            /* 1. Assign stream names with group names that appear in
               commands */
            // map of <streamName, groupName>
            std::map<std::string, std::string> groupMap;
            // a vector of streams in the order they appear
            std::vector<std::pair<std::string, Operation>> streamsInOrder;
            for (const auto &cmd : cfg.commands)
            {
                if (cmd->op == Operation::Write)
                {
                    auto cmdW = dynamic_cast<CommandWrite *>(cmd.get());
                    groupMap[cmdW->streamName] = cmdW->groupName;
                    streamsInOrder.push_back(
                        std::make_pair(cmdW->streamName, Operation::Write));
                }
                else if (cmd->op == Operation::Read)
                {
                    auto cmdR = dynamic_cast<CommandRead *>(cmd.get());
                    groupMap[cmdR->streamName] = cmdR->groupName;
                    streamsInOrder.push_back(
                        std::make_pair(cmdR->streamName, Operation::Read));
                }
            }

            std::map<std::string, std::shared_ptr<ioGroup>> ioMap;

            /* 2. Declare/define groups and open streams in the order they
             * appear */
            std::map<std::string, std::shared_ptr<Stream>> readStreamMap;
            std::map<std::string, std::shared_ptr<Stream>> writeStreamMap;

            for (const auto &st : streamsInOrder)
            {
                const std::string &streamName = st.first;
                std::shared_ptr<ioGroup> io;
                auto &groupName = groupMap[streamName];
                auto it = ioMap.find(groupName);
                if (it == ioMap.end())
                {
                    io = createGroup(groupName, settings.iolib, adios);
                    ioMap[groupName] = io;
                }
                else
                {
                    io = it->second;
                }
                const bool isWrite = (st.second == Operation::Write);
                if (isWrite)
                {
                    auto it = writeStreamMap.find(streamName);
                    if (it == writeStreamMap.end())
                    {
                        std::shared_ptr<Stream> writer =
                            openStream(streamName, io, adios2::Mode::Write,
                                       settings.iolib, settings.appComm);
                        writeStreamMap[streamName] = writer;
                    }
                }
                else /* Read */
                {
                    auto it = readStreamMap.find(streamName);
                    if (it == readStreamMap.end())
                    {
                        std::shared_ptr<Stream> reader =
                            openStream(streamName, io, adios2::Mode::Read,
                                       settings.iolib, settings.appComm);
                        readStreamMap[streamName] = reader;
                    }
                }
            }

            /* Execute commands */
            bool exitLoop = false;
            size_t step = 1;
            while (!exitLoop)
            {
                if (!settings.myRank)
                {
                    std::cout << "Step " << step << ": " << std::endl;
                }
                for (const auto cmd : cfg.commands)
                {
                    if (!cmd->conditionalStream.empty() &&
                        cfg.condMap.at(cmd->conditionalStream) !=
                            adios2::StepStatus::OK)
                    {
                        if (!settings.myRank && settings.verbose)
                        {
                            std::cout << "    Skip command because of status "
                                         "of stream "
                                      << cmd->conditionalStream << std::endl;
                        }
                        continue;
                    }

                    switch (cmd->op)
                    {
                    case Operation::Sleep:
                    {
                        auto cmdS =
                            dynamic_cast<const CommandSleep *>(cmd.get());
                        if (!settings.myRank && settings.verbose)
                        {
                            double t = static_cast<double>(cmdS->sleepTime_us) /
                                       1000000.0;
                            std::cout << "    Sleep for " << t << "  seconds "
                                      << std::endl;
                        }
                        std::this_thread::sleep_for(
                            std::chrono::microseconds(cmdS->sleepTime_us));
                        break;
                    }
                    case Operation::Write:
                    {
                        auto cmdW = dynamic_cast<CommandWrite *>(cmd.get());
                        auto stream = writeStreamMap[cmdW->streamName];
                        // auto io = ioMap[cmdW->groupName];
                        stream->Write(cmdW, cfg, settings, step);
                        break;
                    }
                    case Operation::Read:
                    {
                        auto cmdR = dynamic_cast<CommandRead *>(cmd.get());
                        auto statusIt = cfg.condMap.find(cmdR->streamName);
                        if (statusIt->second == adios2::StepStatus::OK ||
                            statusIt->second == adios2::StepStatus::NotReady)
                        {
                            auto stream = readStreamMap[cmdR->streamName];
                            // auto io = ioMap[cmdR->groupName];
                            adios2::StepStatus status =
                                stream->Read(cmdR, cfg, settings, step);
                            statusIt->second = status;
                            switch (status)
                            {
                            case adios2::StepStatus::OK:
                                break;
                            case adios2::StepStatus::NotReady:
                                if (!settings.myRank && settings.verbose)
                                {
                                    std::cout << "    Nonblocking read status: "
                                                 "Not Ready "
                                              << std::endl;
                                }
                                break;
                            case adios2::StepStatus::EndOfStream:
                            case adios2::StepStatus::OtherError:
                                cfg.stepOverStreams.erase(cmdR->streamName);
                                if (!settings.myRank && settings.verbose)
                                {
                                    std::cout << "    Nonblocking read status: "
                                                 "Terminated "
                                              << std::endl;
                                }
                                break;
                            }
                        }
                        break;
                    }
                    }
                    if (!settings.myRank && settings.verbose)
                    {
                        std::cout << std::endl;
                    }
                }
                if (!cfg.stepOverStreams.size() && step >= cfg.nSteps)
                {
                    exitLoop = true;
                }
                ++step;
            }

            /* Close all streams in order of opening */
            for (const auto &st : streamsInOrder)
            {
                const std::string &streamName = st.first;
                const bool isWrite = (st.second == Operation::Write);
                if (isWrite)
                {
                    auto writerIt = writeStreamMap.find(streamName);
                    if (writerIt != writeStreamMap.end())
                    {
                        auto writer = writeStreamMap[streamName];
                        writerIt->second->Close();
                        writeStreamMap.erase(writerIt);
                    }
                }
                else /* Read */
                {
                    auto readerIt = readStreamMap.find(streamName);
                    if (readerIt != readStreamMap.end())
                    {
                        auto reader = readStreamMap[streamName];
                        readerIt->second->Close();
                        readStreamMap.erase(readerIt);
                    }
                }
            }
        }
        catch (std::exception &e) // config file processing errors
        {
            if (!settings.myRank)
            {
                std::cout << "ADIOS " << e.what() << std::endl;
            }
            MPI_Finalize();
            return 0;
        }
    }

    MPI_Finalize();
    return 0;
}
