#include <iostream>

#include "DumpMan.h"

std::shared_ptr<adios::realtime::DataManBase> getMan()
{
    return std::shared_ptr<adios::realtime::DataManBase>(
        new adios::realtime::DumpMan);
}

namespace adios
{
namespace realtime
{

int DumpMan::init(json p_jmsg)
{
    if (p_jmsg["dumping"] != NULL)
    {
        m_dumping = p_jmsg["dumping"].get<bool>();
    }
    return 0;
}

int DumpMan::get(void *p_data, json &p_jmsg) { return 0; }

int DumpMan::put(const void *p_data, json p_jmsg)
{
    put_begin(p_data, p_jmsg);

    if (!m_dumping)
    {
        return 0;
    }

    std::string doid = p_jmsg["doid"];
    std::string var = p_jmsg["var"];
    std::string dtype = p_jmsg["dtype"];
    std::vector<size_t> putshape =
        p_jmsg["putshape"].get<std::vector<size_t>>();
    std::vector<size_t> varshape =
        p_jmsg["varshape"].get<std::vector<size_t>>();
    std::vector<size_t> offset = p_jmsg["offset"].get<std::vector<size_t>>();
    int numbers_to_print = 100;
    if (numbers_to_print > product(putshape, 1))
        ;
    size_t putsize = product(putshape, dsize(dtype));

    std::cout << p_jmsg.dump(4) << std::endl;
    std::cout << "total MBs = " << product(putshape, dsize(dtype)) / 1000000
              << std::endl;

    void *data_to_dump;

    void *data = malloc(putsize);
    if (auto_transform(p_data, data, p_jmsg))
    {
        data_to_dump = data;
    }
    else
    {
        data_to_dump = const_cast<void *>(p_data);
    }

    if (dtype == "float")
        for (size_t i = 0; i < numbers_to_print; i++)
            std::cout << ((float *)data_to_dump)[i] << " ";
    if (dtype == "double")
        for (size_t i = 0; i < numbers_to_print; i++)
            std::cout << ((double *)data_to_dump)[i] << " ";

    std::cout << std::endl;

    put_end(data, p_jmsg);
    free(data);

    return 0;
}

void DumpMan::flush() {}

} // end namespace realtime
} // end namespace adios
