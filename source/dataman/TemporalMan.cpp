/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TemporalMan.cpp
 *
 *  Created on: Apr 20, 2017
 *      Author: Jason Wang
 */

#include "TemporalMan.h"

int TemporalMan::init(json p_jmsg) { return 0; }

int TemporalMan::put(const void *p_data, json p_jmsg)
{
    put_begin(p_data, p_jmsg);
    put_end(p_data, p_jmsg);
    return 0;
}

int TemporalMan::get(void *p_data, json &p_jmsg) { return 0; }

void TemporalMan::flush() {}

void TemporalMan::transform(std::vector<char> &a_data, json &a_jmsg) {}
