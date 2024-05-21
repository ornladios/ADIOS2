#include "AdiosFilePool.h"

namespace adios2
{

AnonADIOSFile *ADIOSFilePool::GetFree(std::string Filename, bool RowMajorArrays)
{
    std::lock_guard<std::mutex> guard(pool_mutex);
    std::string index = Filename + "/" + std::to_string((int)RowMajorArrays);
    auto res = map.find(index);

    SubPool *subpool;
    AnonADIOSFile *ret;
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

ADIOSFilePool::ADIOSFilePool() {}

void ADIOSFilePool::FlushUnused()
{
    for (auto it = map.cbegin(), next_it = it; it != map.cend(); it = next_it)
    {
        ++next_it;
        auto subpool = it->second.get();
        bool should_delete = (subpool->in_use_count == 0);

        if (should_delete)
        {
            std::cout << "Releasing subpool" << std::endl;
            map.erase(it);
        }
    }
}

ADIOSFilePool::~ADIOSFilePool() {}
} // end of namespace adios2
