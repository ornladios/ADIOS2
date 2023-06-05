/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CampaignManager.h
 * Campaign manager object created by the ADIOS object as GlobalService.
 * Record all output under the ADIOS object so that CampaignReader can
 * see all data for as a single reader.
 * This is NOT a writer Engine but the CampaignReader is a reader Engine.
 *
 *  Created on: May 15, 2023
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#ifndef ADIOS2_ENGINE_CAMPAIGNMANAGER_H_
#define ADIOS2_ENGINE_CAMPAIGNMANAGER_H_

#include "CampaignData.h"
#include "adios2/common/ADIOSConfig.h"
#include "adios2/helper/adiosComm.h"

#include <fstream>

namespace adios2
{
namespace core
{

namespace engine
{

class CampaignManager
{

public:
    CampaignManager(helper::Comm &comm);
    ~CampaignManager();

    void Open(const std::string &name);
    void Record(const std::string &name, const size_t step, const double time);
    void Close();

private:
    bool m_Opened = false;
    std::string m_Name;
    int m_WriterRank;
    int m_Verbosity = 5;
    CampaignMap cmap;
    std::ofstream m_Output;
    const std::string m_CampaignDir = "adios-campaign";
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_CAMPAIGNMANAGER_H_ */
