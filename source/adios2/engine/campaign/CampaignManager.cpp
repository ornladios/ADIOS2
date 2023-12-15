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

#include <nlohmann_json.hpp>

namespace adios2
{
namespace core
{
namespace engine
{

static std::string CMapToJson(const CampaignRecordMap &cmap, const int rank, const std::string name)
{
    nlohmann::json j = nlohmann::json::array();
    for (auto &r : cmap)
    {
        nlohmann::json c = nlohmann::json{{"name", r.first},
                                          {"varying_deltas", r.second.varying_deltas},
                                          {"delta_step", r.second.delta_step},
                                          {"delta_time", r.second.delta_time},
                                          {"steps", r.second.steps},
                                          {"times", r.second.times}};
        j.push_back(c);
    }
    return nlohmann::to_string(j);
}

CampaignManager::CampaignManager(adios2::helper::Comm &comm)
{
    m_WriterRank = comm.Rank();
    if (m_Verbosity == 5)
    {
        std::cout << "Campaign Manager " << m_WriterRank << " constructor called" << std::endl;
    }
    helper::CreateDirectory(m_CampaignDir);
}

CampaignManager::~CampaignManager()
{
    if (m_Verbosity == 5)
    {
        std::cout << "Campaign Manager " << m_WriterRank << " desctructor called\n";
    }
    if (m_Opened)
    {
        Close();
    }
}

void CampaignManager::Open(const std::string &name)
{
    m_Name = m_CampaignDir + "/" + name + "_" + std::to_string(m_WriterRank) + ".json";
    if (m_Verbosity == 5)
    {
        std::cout << "Campaign Manager " << m_WriterRank << " Open(" << m_Name << ")\n";
    }
}

void CampaignManager::Record(const std::string &name, const size_t step, const double time)
{
    if (m_Verbosity == 5)
    {
        std::cout << "Campaign Manager " << m_WriterRank << "   Record name = " << name
                  << " step = " << step << " time = " << time << "\n";
    }
    auto r = cmap.find(name);
    if (r != cmap.end())
    {
        // update record
        size_t last_step = r->second.steps.back();
        size_t delta_step = step - last_step;
        double last_time = r->second.times.back();
        double delta_time = time - last_time;
        auto nsteps = r->second.steps.size();
        if (nsteps == 1)
        {
            r->second.delta_step = delta_step;
            r->second.delta_time = delta_time;
        }
        else
        {
            size_t old_delta_step = r->second.steps.back() - r->second.steps.rbegin()[1];
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
    if (!cmap.empty())
    {
        m_Output.open(m_Name, std::ofstream::out);
        m_Opened = true;
        m_Output << std::setw(4) << CMapToJson(cmap, m_WriterRank, m_Name) << std::endl;
        m_Output.close();
        m_Opened = false;
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
