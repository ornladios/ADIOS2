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
#include <sqlite3.h>

namespace adios2
{
namespace core
{
namespace engine
{

int CMapToSqlite(const CampaignRecordMap &cmap, const int rank, std::string name)
{
    sqlite3 *db;
    int rc;
    char *zErrMsg = 0;
    std::string sqlcmd;
    std::string db_name = name + ".db";
    rc = sqlite3_open(db_name.c_str(), &db);

    if (rc != SQLITE_OK)
    {
        std::cout << "SQL error: " << zErrMsg << std::endl;
        std::string m(zErrMsg);
        helper::Throw<std::invalid_argument>("Engine", "CampaignReader", "WriteCampaignData",
                                             "SQL error on writing records:");
        sqlite3_free(zErrMsg);
    }
    sqlcmd = "CREATE TABLE bpfiles (name);";
    rc = sqlite3_exec(db, sqlcmd.c_str(), 0, 0, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        std::cout << "SQL error: " << zErrMsg << std::endl;
        std::string m(zErrMsg);
        helper::Throw<std::invalid_argument>("Engine", "CampaignReader", "WriteCampaignData",
                                             "SQL error on writing records:");
        sqlite3_free(zErrMsg);
    }

    //    sqlcmd = "CREATE TABLE data (name);";
    //    rc = sqlite3_exec(db, sqlcmd.c_str(), 0, 0, &zErrMsg);
    //    if (rc != SQLITE_OK)
    //    {
    //        std::cout << "SQL error: " << zErrMsg << std::endl;
    //        std::string m(zErrMsg);
    //        helper::Throw<std::invalid_argument>("Engine", "CampaignReader", "WriteCampaignData",
    //                                             "SQL error on writing records:");
    //        sqlite3_free(zErrMsg);
    //    }

    size_t rowid = 1000;
    for (auto &r : cmap)
    {
        // vectors r.second.steps and  std::to_string(r.second.times) should go to another table
        // check for NaNs
        double delta_time = 0.0;
        if (!std::isnan(r.second.delta_time))
            delta_time = r.second.delta_time;
        size_t delta_step = 0;
        if (!std::isnan(r.second.delta_step))
            delta_time = r.second.delta_step;
        //        sqlcmd = "INSERT INTO files (rowid, name, varying_deltas, delta_step,
        //        delta_time)\n"; sqlcmd += "VALUES(" + std::to_string(rowid) + "," + "'" + r.first
        //        + "'" + "," +
        //                  std::to_string(r.second.varying_deltas) + "," +
        //                  std::to_string(delta_step) + "," + std::to_string(delta_time) + ");";
        sqlcmd = "INSERT INTO bpfiles (rowid, name)\n";
        sqlcmd += "VALUES(" + std::to_string(rowid) + "," + "'" + r.first + "'" + ");";
        rowid++;
        std::cout << sqlcmd << std::endl;
        rc = sqlite3_exec(db, sqlcmd.c_str(), 0, 0, &zErrMsg);
        if (rc != SQLITE_OK)
        {
            std::cout << "SQL error: " << zErrMsg << std::endl;
            std::string m(zErrMsg);
            helper::Throw<std::invalid_argument>("Engine", "CampaignReader", "WriteCampaignData",
                                                 "SQL error on writing records:");
            sqlite3_free(zErrMsg);
        }
    }

    sqlite3_close(db);

    return 0;
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
    m_Name = m_CampaignDir + "/" + name + "_" + std::to_string(m_WriterRank);
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
        CMapToSqlite(cmap, m_WriterRank, m_Name);
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
