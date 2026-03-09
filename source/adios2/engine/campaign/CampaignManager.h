/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_ENGINE_CAMPAIGNMANAGER_H_
#define ADIOS2_ENGINE_CAMPAIGNMANAGER_H_

#include "adios2/common/ADIOSConfig.h"
#include "adios2/common/ADIOSTypes.h"
#include "adios2/helper/adiosComm.h"

#ifdef ADIOS2_HAVE_CAMPAIGN
#include "CampaignRecord.h"

#include <fstream>
#endif /* ADIOS2_HAVE_CAMPAIGN */

namespace adios2
{
namespace core
{

namespace engine
{

class CampaignManager
{
#ifdef ADIOS2_HAVE_CAMPAIGN
public:
    CampaignManager(helper::Comm &comm);
    ~CampaignManager();

    void Open(const std::string &name, const UserOptions &options);
    void Record(const std::string &name, const size_t step, const double time);
    void Close();

private:
    UserOptions::Campaign m_Options;
    bool m_Opened = false;
    std::string m_Name;
    int m_WriterRank;
    CampaignRecordMap cmap;
    std::ofstream m_Output;
    const std::string m_CampaignDir = ".adios-campaign";

#else
public:
    CampaignManager(helper::Comm &comm){};
    ~CampaignManager() = default;
    void Open(const std::string &name, const UserOptions &options){};
    void Record(const std::string &name, const size_t step, const double time){};
    void Close(){};

#endif /* ADIOS2_HAVE_CAMPAIGN */
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_CAMPAIGNMANAGER_H_ */
