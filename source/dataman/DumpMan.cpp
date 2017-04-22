/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DumpMan.cpp
 *
 *  Created on: Apr 20, 2017
 *      Author: Jason Wang
 */

#include "DumpMan.h"

int DumpMan::init(json p_jmsg)
{
    if (p_jmsg["dumping"].is_boolean())
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
        return 1;
    }
    if (!check_json(p_jmsg, {"doid", "var", "dtype", "putshape"}))
    {
        return -1;
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
    {
        numbers_to_print = product(putshape, 1);
    }
    size_t putbytes = product(putshape, dsize(dtype));

    std::cout << p_jmsg.dump(4) << std::endl;
    std::cout << "total MBs = " << product(putshape, dsize(dtype)) / 1000000
              << std::endl;

    const void *data_to_dump;

    std::vector<char> data(putbytes);

    if (auto_transform(p_data, data.data(), p_jmsg))
    {
        data_to_dump = data.data();
    }
    else
    {
        data_to_dump = p_data;
    }

    for (size_t i = 0; i < numbers_to_print; i++)
    {
        if (dtype == "float")
        {
            std::cout << static_cast<const float *>(data_to_dump)[i] << " ";
        }
        if (dtype == "double")
        {
            std::cout << static_cast<const double *>(data_to_dump)[i] << " ";
        }
    }

    std::cout << std::endl;
    put_end(p_data, p_jmsg);
    return 0;
}

void DumpMan::flush() {}
