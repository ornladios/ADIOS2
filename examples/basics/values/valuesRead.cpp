/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Write single values to a file. There are four different cases:
 * 1. Global constant - same on all processes, constant over time
 * 2. Global value - same on all processes, may change over time
 * 3. Local constants - different across processes, constant over time
 * 4. Local value - different across processes, may change over time
 *
 * Constants are not handled separately from time-varying values in ADIOS.
 * Simply write them only in the first step.
 *
 * Writing a global value from multiple processes does not hurt but it is
 * useless.
 *
 * Created on: Jun 2, 2017
 *      Author: pnorbert
 */

#include <iostream>
#include <string>
#include <thread> // sleep_for
#include <vector>

#include <adios2.h>
#include <adios2/helper/adiosString.h>

int main(int argc, char *argv[])
{
    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("Input");
    // io.SetEngine("SST");
    adios2::Engine reader = io.Open("values.bp", adios2::Mode::Read);

    while (true)
    {
        auto status = reader.BeginStep();
        if (status == adios2::StepStatus::NotReady)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }
        if (status != adios2::StepStatus::OK)
        {
            break;
        }

        std::cout << "---- Step " << reader.CurrentStep() << " ----" << std::endl;

        // all-string map of all variables for easy discovery
        auto vars = io.AvailableVariables();
        std::cout << "  List variables:" << std::endl;
        for (auto &v : vars)
        {
            const std::string &varname = v.first;
            const std::string &vartype = v.second["Type"];
            const std::string &varshape = v.second["Shape"];
            const std::string &varsgl = v.second["SingleValue"];
            if (varsgl == "true")
            {
                std::cout << "    " << vartype << " " << varname << " is a single value "
                          << std::endl;
            }

            else
            {
                std::cout << "    " << vartype << " " << varname << " shape = " << varshape
                          << std::endl;
            }
        }

        // GlobalValue is a single value (per step)
        // see: bpls  -l values.bp/ -d Nproc
        auto vNproc = io.InquireVariable<int>("Nproc");
        if (vNproc)
        {
            int value;
            reader.Get(vNproc, &value, adios2::Mode::Sync);
            std::cout << "  Nproc = " << value << std::endl;
        }

        // see: bpls  -l values.bp/ -d GlobalString -n 1
        auto vGS = io.InquireVariable<std::string>("GlobalString");
        if (vGS)
        {
            std::string value;
            reader.Get(vGS, &value, adios2::Mode::Sync);
            std::cout << "  GlobalString = " << value << std::endl;
        }

        // Step is a single value but in many steps
        // see: bpls  -l values.bp/ -d Step
        auto vStep = io.InquireVariable<int>("Step");
        if (vStep)
        {
            int value;
            reader.Get(vStep, &value, adios2::Mode::Sync);
            std::cout << "  Step = " << value << std::endl;
        }

        // LocalValue across processes shows up as 1D array when reading
        // see: bpls  -l values.bp/ -d ProcessID
        auto vRank = io.InquireVariable<int>("ProcessID");
        if (vRank)
        {
            std::cout << "  ProcessID:" << std::endl;
            adios2::Dims shape = vRank.Shape();
            adios2::Dims count = vRank.Count();
            std::cout << "    shape = " << adios2::helper::DimsToString(shape) << std::endl;
            std::cout << "    count = " << adios2::helper::DimsToString(count) << std::endl;
            std::vector<int> pids;
            reader.Get(vRank, pids, adios2::Mode::Sync);
            std::cout << "    content = [ ";
            for (int pid : pids)
            {
                std::cout << pid << " ";
            }
            std::cout << "]" << std::endl;

            // list all blocks (writer's Put's one by one)
            auto bis = reader.AllStepsBlocksInfo(vRank);
            auto bi = bis[0]; // we know we wrote it only in step 0
            for (size_t block = 0; block < bi.size(); ++block)
            {
                int value;
                vRank.SetBlockSelection(block);
                reader.Get(vRank, &value, adios2::Mode::Sync);
                std::cout << "      block " << block << " = " << value << std::endl;
            }
        }

        // Strings cannot be represented as 1D array, so the only option is to read block-by-block
        // see: bpls  -l values.bp/ -d LocalString -D
        auto vLS = io.InquireVariable<std::string>("LocalString");
        if (vLS)
        {
            std::cout << "  LocalString:" << std::endl;
            auto bis = reader.AllStepsBlocksInfo(vLS);
            auto bi = bis[0]; // we know we wrote it only in step 0

            std::cout << "    Read as 1D array into std::vector<std::string>:" << std::endl;
            std::vector<std::string> strings;
            reader.Get(vLS, strings, adios2::Mode::Sync);
            for (size_t block = 0; block < strings.size(); ++block)
            {
                std::cout << "        block " << block << " = " << strings[block] << std::endl;
            }

            // This metadata contains all values in string form already
            std::cout << "    Accessed block-by-block from metadata directly:" << std::endl;
            for (size_t block = 0; block < bi.size(); ++block)
            {
                std::string value;
                vLS.SetBlockSelection(block);
                reader.Get(vLS, &value, adios2::Mode::Sync);
                std::cout << "        block " << block << " = " << bi[block].Value << std::endl;
            }

            // Get the individual values using standard interface Engine::Get()
            // using Variable::SetBlockSelection
            std::cout << "    Read block-by-block using SetBlockSelection:" << std::endl;
            for (size_t block = 0; block < bi.size(); ++block)
            {
                std::string value;
                vLS.SetBlockSelection(block);
                reader.Get(vLS, &value, adios2::Mode::Sync);
                std::cout << "        block " << block << " = " << value << std::endl;
            }
        }

        // GlobalStringEveryoneWrites is a single string but written by multiple processes
        // The only option is to read block-by-block either from metadata or with SetBlockSelection
        // see: bpls -l values.bp/ -d GlobalStringEveryoneWrites -D -n 1
        auto vGSEW = io.InquireVariable<std::string>("GlobalStringEveryoneWrites");
        if (vGSEW)
        {
            std::cout << "GlobalStringEveryoneWrites:" << std::endl;
            auto bis = reader.AllStepsBlocksInfo(vGSEW);
            auto bi = bis[0]; // we know we wrote it only in step 0

            // This metadata contains all values in string form already
            std::cout << "    Accessed block-by-block from metadata directly:" << std::endl;
            for (size_t block = 0; block < bi.size(); ++block)
            {
                std::string value;
                vGSEW.SetBlockSelection(block);
                reader.Get(vGSEW, &value, adios2::Mode::Sync);
                std::cout << "        block " << block << " = " << bi[block].Value << std::endl;
            }

            // Get the individual values using standard interface Engine::Get()
            // using Variable::SetBlockSelection
            std::cout << "    Read block-by-block using SetBlockSelection:" << std::endl;
            for (size_t block = 0; block < bi.size(); ++block)
            {
                std::string value;
                vGSEW.SetBlockSelection(block);
                reader.Get(vGSEW, &value, adios2::Mode::Sync);
                std::cout << "        block " << block << " = " << value << std::endl;
            }
        }

        // Nparts is a 1D array over steps
        // See: bpls  -l values.bp -d Nparts  -n <N>
        //      where N is the number of processes you run the example with
        auto vNparts = io.InquireVariable<uint32_t>("Nparts");
        if (vNparts)
        {
            std::cout << "  Nparts:" << std::endl;
            adios2::Dims shape = vNparts.Shape();
            adios2::Dims count = vNparts.Count();
            std::cout << "    shape = " << adios2::helper::DimsToString(shape) << std::endl;
            std::cout << "    count = " << adios2::helper::DimsToString(count) << std::endl;

            // We know that we wrote same number of values every step
            // so we can read all of them at once
            std::vector<uint32_t> pids;
            reader.Get(vNparts, pids, adios2::Mode::Sync);
            std::cout << "    content = [";
            for (auto pid : pids)
            {
                std::cout << pid << " ";
            }
            std::cout << "]" << std::endl;

            reader.EndStep();
        }
    }
    reader.Close();
    return 0;
}