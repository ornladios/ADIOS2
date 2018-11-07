/*
 * adiosStream.cpp
 *
 *  Created on: Nov 2018
 *      Author: Norbert Podhorszki
 */

#include "adiosStream.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <math.h>
#include <stdexcept>
#include <string>

adiosStream::adiosStream(const std::string &streamName, adios2::IO &io,
                         const adios2::Mode mode, MPI_Comm comm)
: Stream(streamName, mode), io(io)
{

    if (mode == adios2::Mode::Write)
    {
        engine = io.Open(streamName, adios2::Mode::Write, comm);
    }
    else
    {
        engine = io.Open(streamName, adios2::Mode::Read, comm);
    }
}

adiosStream::~adiosStream(){};

void adiosStream::defineADIOSArray(const std::shared_ptr<VariableInfo> ov)
{
    if (ov->type == "double")
    {
        adios2::Variable<double> v = io.DefineVariable<double>(
            ov->name, ov->shape, ov->start, ov->count, true);
        // v = io->InquireVariable<double>(ov->name);
    }
    else if (ov->type == "float")
    {
        adios2::Variable<float> v = io.DefineVariable<float>(
            ov->name, ov->shape, ov->start, ov->count, true);
    }
    else if (ov->type == "int")
    {
        adios2::Variable<int> v = io.DefineVariable<int>(
            ov->name, ov->shape, ov->start, ov->count, true);
    }
}

void adiosStream::putADIOSArray(const std::shared_ptr<VariableInfo> ov)
{
    if (ov->type == "double")
    {
        const double *a = reinterpret_cast<const double *>(ov->data.data());
        engine.Put<double>(ov->name, a);
    }
    else if (ov->type == "float")
    {
        const float *a = reinterpret_cast<const float *>(ov->data.data());
        engine.Put<float>(ov->name, a);
    }
    else if (ov->type == "int")
    {
        const int *a = reinterpret_cast<const int *>(ov->data.data());
        engine.Put<int>(ov->name, a);
    }
}

void adiosStream::getADIOSArray(std::shared_ptr<VariableInfo> ov)
{
    if (ov->type == "double")
    {
        adios2::Variable<double> v = io.InquireVariable<double>(ov->name);
        if (!v)
        {
            ov->readFromInput = false;
            return;
        }
        v.SetSelection({ov->start, ov->count});
        double *a = reinterpret_cast<double *>(ov->data.data());
        engine.Get<double>(v, a);
        ov->readFromInput = true;
    }
    else if (ov->type == "float")
    {
        adios2::Variable<float> v = io.InquireVariable<float>(ov->name);
        if (!v)
        {
            ov->readFromInput = false;
            return;
        }
        v.SetSelection({ov->start, ov->count});
        float *a = reinterpret_cast<float *>(ov->data.data());
        engine.Get<float>(v, a);
        ov->readFromInput = true;
    }
    else if (ov->type == "int")
    {
        adios2::Variable<int> v = io.InquireVariable<int>(ov->name);
        if (!v)
        {
            ov->readFromInput = false;
            return;
        }
        v.SetSelection({ov->start, ov->count});
        int *a = reinterpret_cast<int *>(ov->data.data());
        engine.Get<int>(v, a);
        ov->readFromInput = true;
    }
}

/* return true if read-in completed */
adios2::StepStatus adiosStream::readADIOS(CommandRead *cmdR, Config &cfg,
                                          const Settings &settings, size_t step)
{
    if (!settings.myRank && settings.verbose)
    {
        std::cout << "    Read ";
        if (cmdR->stepMode == adios2::StepMode::NextAvailable)
        {
            std::cout << "next available step from ";
        }
        else
        {
            std::cout << "latest step from ";
        }

        std::cout << cmdR->streamName << " using the group " << cmdR->groupName;
        if (!cmdR->variables.empty())
        {
            std::cout << " with selected variables:  ";
            for (const auto &v : cmdR->variables)
            {
                std::cout << v->name << " ";
            }
        }
        std::cout << std::endl;
    }
    adios2::StepStatus status =
        engine.BeginStep(cmdR->stepMode, cmdR->timeout_sec);
    if (status != adios2::StepStatus::OK)
    {
        return status;
    }

    if (!settings.myRank && settings.verbose && step == 1)
    {
        const auto varmap = io.AvailableVariables();
        std::cout << "    Variables in input for reading: " << std::endl;
        for (const auto &v : varmap)
        {
            std::cout << "        " << v.first << std::endl;
        }
    }

    if (!settings.myRank && settings.verbose)
    {
        std::cout << "    Read data " << std::endl;
    }

    for (auto ov : cmdR->variables)
    {
        getADIOSArray(ov);
    }
    engine.EndStep();
    return status;
}

void adiosStream::writeADIOS(CommandWrite *cmdW, Config &cfg,
                             const Settings &settings, size_t step)
{
    if (!settings.myRank && settings.verbose)
    {
        std::cout << "    Write to output " << cmdW->streamName << " the group "
                  << cmdW->groupName;
        if (!cmdW->variables.empty())
        {
            std::cout << " with selected variables:  ";
            for (const auto &v : cmdW->variables)
            {
                std::cout << v->name << " ";
            }
        }
        std::cout << std::endl;
    }

    const double div =
        pow(10.0, static_cast<const double>(settings.ndigits(cfg.nSteps - 1)));
    double myValue = static_cast<double>(settings.myRank) +
                     static_cast<double>(step - 1) / div;

    std::map<std::string, adios2::Params> definedVars = io.AvailableVariables();
    for (auto ov : cmdW->variables)
    {
        // if the variable is not in the IO group it means
        // we have not defined it yet (e.g. a write-only variable or a linked
        // variable defined in another read group)
        const auto it = definedVars.find(ov->name);
        if (it == definedVars.end())
        {
            if (!settings.myRank && settings.verbose)
            {
                std::cout << "        Define array  " << ov->name
                          << "  for output" << std::endl;
            }
            defineADIOSArray(ov);
        }

        // if we read the variable, use the read values otherwise generate data
        // now
        if (!ov->readFromInput)
        {
            if (!settings.myRank && settings.verbose)
            {
                std::cout << "        Fill array  " << ov->name
                          << "  for output" << std::endl;
            }
            fillArray(ov, myValue);
        }
    }

    if (!settings.myRank && settings.verbose)
    {
        std::cout << "        Write data " << std::endl;
    }
    engine.BeginStep();
    for (const auto ov : cmdW->variables)
    {
        putADIOSArray(ov);
    }
    engine.EndStep();
}

void adiosStream::Write(CommandWrite *cmdW, Config &cfg,
                        const Settings &settings, size_t step)
{

    writeADIOS(cmdW, cfg, settings, step);
}

adios2::StepStatus adiosStream::Read(CommandRead *cmdR, Config &cfg,
                                     const Settings &settings, size_t step)
{
    return readADIOS(cmdR, cfg, settings, step);
}

void adiosStream::Close() { engine.Close(); }
