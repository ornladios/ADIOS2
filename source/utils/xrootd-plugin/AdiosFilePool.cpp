#include "AdiosFilePool.h"
#include <iostream>
#include <thread>

namespace adios2
{

AnonADIOSFile *ADIOSFilePool::GetFree(std::string Filename, bool RowMajorArrays)
{
    std::lock_guard<std::mutex> guard(pool_mutex);
    std::string index = Filename + "/" + std::to_string((int)RowMajorArrays);
    auto res = map.find(index);

    SubPool *subpool;
    if (res != map.end())
    {
        subpool = res->second.get();
    }
    else
    {
        std::unique_ptr<SubPool> tmp(new SubPool());
        subpool = tmp.get();
        map.insert(std::make_pair(index, std::move(tmp)));
    }
    return subpool->GetFree(Filename, RowMajorArrays);
}

void ADIOSFilePool::Return(AnonADIOSFile *Entry)
{
    std::lock_guard<std::mutex> guard(pool_mutex);
    std::string index = Entry->m_FileName + "/" + std::to_string((int)Entry->m_RowMajorArrays);
    auto res = map.find(index);
    auto subpool = res->second.get();
    subpool->Return(Entry);
}

AnonADIOSFile *ADIOSFilePool::SubPool::GetFree(std::string Filename, bool RowMajorArrays)
{
    for (size_t i = 0; i < m_busy.size(); i++)
    {
        if (!m_busy[i])
        {
            m_busy[i] = true;
            in_use_count++;
            return m_list[i].get();
        }
    }
    // no free files
    m_list.push_back(std::unique_ptr<AnonADIOSFile>(new AnonADIOSFile(Filename, RowMajorArrays)));
    m_busy.push_back(true);
    in_use_count++;
    return m_list[m_list.size() - 1].get();
}
void ADIOSFilePool::SubPool::Return(adios2::AnonADIOSFile *to_free)
{
    last_used = std::chrono::steady_clock::now();
    for (size_t i = 0; i < m_busy.size(); i++)
    {
        if (to_free == m_list[i].get())
        {
            m_busy[i] = false;
            in_use_count--;
            return;
        }
    }
    std::cerr << "Return FAILED " << std::endl;
}

void ADIOSFilePool::DoTimeoutTasks() { FlushUnused(); }

void ADIOSFilePool::PeriodicTask(std::chrono::milliseconds interval)
{
    while (!(m_ShutdownFlag))
    {
        DoTimeoutTasks();
        std::this_thread::sleep_for(interval);
    }
    std::cout << "Clean shutdown of ADIOSFilePool periodic thread" << std::endl;
}

ADIOSFilePool::ADIOSFilePool()
{
    std::cout << "Singleton ADIOSFilePool instance created.\n";
    periodicThread = periodicWorker();
}

ADIOSFilePool &ADIOSFilePool::getInstance()
{
    static ADIOSFilePool instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
}

void ADIOSFilePool::FlushUnused()
{
    auto now = std::chrono::steady_clock::now();
    for (auto it = map.cbegin(), next_it = it; it != map.cend(); it = next_it)
    {
        ++next_it;
        auto subpool = it->second.get();
        auto unused_duration = now - subpool->last_used;
        auto elapsed_seconds =
            std::chrono::duration_cast<std::chrono::seconds>(unused_duration).count();
        bool should_delete = (elapsed_seconds > 15) && (subpool->in_use_count == 0);
        std::string fname = "";
        if (subpool->m_list.size() > 0)
            fname = subpool->m_list[0]->m_FileName;
        if (should_delete)
        {
            if (subpool->m_list.size() > 0)
                std::cout << "Releasing subpool for file \"" << subpool->m_list[0]->m_FileName
                          << "\" unused for " << elapsed_seconds << " seconds" << std::endl;
            map.erase(it);
        }
    }
}

ADIOSFilePool::~ADIOSFilePool()
{
    m_ShutdownFlag = true;
    if (periodicThread.joinable())
    {
        periodicThread.join();
    }
}
} // end of namespace adios2
