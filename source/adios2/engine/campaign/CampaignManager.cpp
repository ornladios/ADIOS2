/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CampaignManager.cpp
 *
 * This is NOT a writer Engine but the CampaignReader is a reader Engine.
 *
 *  Created on: May 15, 2023
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#include "CampaignManager.h"

#include "adios2/helper/adiosFunctions.h"

#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

CampaignManager::CampaignManager(adios2::helper::Comm &comm)
{
    m_WriterRank = comm.Rank();
    if (m_Verbosity == 5)
    {
        std::cout << "Campaign Manager " << m_WriterRank
                  << " constructor called" << std::endl;
    }
}

CampaignManager::~CampaignManager()
{
    if (m_Verbosity == 5)
    {
        std::cout << "Campaign Manager " << m_WriterRank
                  << " desctructor called\n";
    }
    if (m_Opened)
    {
        Close();
    }
}

void CampaignManager::Open(const std::string &name)
{
    if (m_Verbosity == 5)
    {
        std::cout << "Campaign Manager " << m_WriterRank << " Open(" << name
                  << ")\n";
    }
    m_Opened = true;
}

void CampaignManager::Record(const std::string &name, const size_t step,
                             const double time)
{
    if (m_Verbosity == 5)
    {
        std::cout << "Campaign Manager " << m_WriterRank
                  << "   Record name = " << name << " step = " << step
                  << " time = " << time << "\n";
    }
    auto r = cmap.find(name);
    if (r != cmap.end())
    {
        // update record
        size_t last_step = r->second.steps.back();
        size_t delta_step = step - last_step;
        double last_time = r->second.times.back();
        double delta_time = time - last_time;
        int nsteps = r->second.steps.size();
        if (nsteps == 1)
        {
            r->second.delta_step = delta_step;
            r->second.delta_time = delta_time;
        }
        else
        {
            size_t old_delta_step =
                r->second.steps.back() - r->second.steps.rbegin()[1];
            if (old_delta_step != delta_step)
            {
                r->second.delta_step = 0;
                r->second.delta_time = 0.0;
                r->second.varying_deltas = true;
            }
        }
        r->second.steps.push_back(step);
        r->second.times.push_back(time);
    }
    else
    {
        // new entry
        CampaignRecord r(step, time);
        cmap.emplace(name, r);
    }
}

void CampaignManager::Close()
{
    if (m_Verbosity == 5)
    {
        std::cout << "Campaign Manager " << m_WriterRank
                  << " Close()\nCampaign";
        for (const auto &r : cmap)
        {
            std::cout << "name = " << r.first << "\n";
            std::cout << "   varying_deltas = " << r.second.varying_deltas
                      << "\n";
            std::cout << "   delta_step     = " << r.second.delta_step << "\n";
            std::cout << "   delta_time     = " << r.second.delta_time << "\n";
            std::cout << "   steps          = {";
            for (const auto s : r.second.steps)
            {
                std::cout << s << " ";
            }
            std::cout << "}\n";
            std::cout << "   times          = {";
            for (const auto t : r.second.times)
            {
                std::cout << t << " ";
            }
            std::cout << "}\n";
        }
    }
    m_Opened = false;
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
