/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CampaignManager.cpp
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
}

void CampaignManager::Close()
{
    if (m_Verbosity == 5)
    {
        std::cout << "Campaign Manager " << m_WriterRank << " Close()\n";
    }
    m_Opened = false;
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
