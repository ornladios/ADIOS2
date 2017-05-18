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

int DumpMan::init(json a_jmsg)
{
    if (a_jmsg["dumping"].is_boolean())
    {
        m_dumping = a_jmsg["dumping"].get<bool>();
    }
    return 0;
}
int DumpMan::get(void *a_data, json &a_jmsg) { return 0; }

int DumpMan::put(const void *a_data, json &a_jmsg)
{
    put_begin(a_data, a_jmsg);

    if (!m_dumping)
    {
        return 1;
    }
    if (!check_json(a_jmsg, {"doid", "var", "dtype", "putshape"}))
    {
        return -1;
    }

    std::string doid = a_jmsg["doid"];
    std::string var = a_jmsg["var"];
    std::string dtype = a_jmsg["dtype"];
    std::vector<size_t> putshape =
        a_jmsg["putshape"].get<std::vector<size_t>>();
    std::vector<size_t> varshape =
        a_jmsg["varshape"].get<std::vector<size_t>>();
    std::vector<size_t> offset = a_jmsg["offset"].get<std::vector<size_t>>();
    int numbers_to_print = 100;
    if (numbers_to_print > product(putshape, 1))
    {
        numbers_to_print = product(putshape, 1);
    }
    size_t putbytes = product(putshape, dsize(dtype));
    size_t sendbytes = a_jmsg["sendbytes"].get<size_t>();

    std::cout << a_jmsg.dump(4) << std::endl;
    std::cout << "total MBs = " << product(putshape, dsize(dtype)) / 1000000
              << std::endl;

    std::vector<char> data(static_cast<const char *>(a_data),
                           static_cast<const char *>(a_data) + sendbytes);

    auto_transform(data, a_jmsg);

    void *data_to_print = data.data();
    for (size_t i = 0; i < numbers_to_print; ++i)
    {
        if (dtype == "float")
        {
            std::cout << static_cast<float *>(data_to_print)[i] << " ";
        }
        if (dtype == "double")
        {
            std::cout << static_cast<double *>(data_to_print)[i] << " ";
        }
    }

    std::cout << std::endl;
    put_end(a_data, a_jmsg);
    return 0;
}

void DumpMan::flush() {}
