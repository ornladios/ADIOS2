/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosRangeFilter.cpp
 *
 *  Created on: Feb 4, 2021
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#include "adiosRangeFilter.h"

/// \cond EXCLUDE_FROM_DOXYGEN
//#include <iostream>
#include <sstream>
#include <stdexcept> // std::invalid_argument
/// \endcond

#include "adios2/helper/adiosString.h"

namespace adios2
{
namespace helper
{

void RangeFilter::ParseSelection(const std::string &selection)
{
    std::stringstream ss(selection);
    std::string item;

    m_Selection.clear();
    m_UnlimitedRules.clear();

    while (std::getline(ss, item, ' '))
    {
        if (!item.empty())
        {
            // std::cout << "  Parse def [" << item << "]" << std::endl;
            std::stringstream ss(item);
            std::string start, end, steps;
            std::getline(ss, start, ':');
            std::getline(ss, end, ':');
            if (!end.empty())
            {
                std::getline(ss, steps, ':');
            }
            /*std::cout << "  start [" << start << "]"
                      << "  end [" << end << "]"
                      << "  steps [" << steps << "]" << std::endl;*/

            size_t n = ToSizeT(start);
            size_t m = n;
            if (!end.empty())
            {
                if (end == "n" || end == "N")
                {
                    m = MaxSizeT;
                }
                else
                {
                    m = ToSizeT(end);
                }
            }

            size_t s = 1;
            if (!steps.empty())
            {
                s = ToSizeT(steps);
            }
            /*std::cout << "  start = " << n << "  end = " << m
                      << "  steps = " << s << std::endl;*/

            if (m < MaxSizeT)
            {
                if (m_Selection.size() < m + 1)
                {
                    m_Selection.resize(m + 1);
                }
                for (size_t i = n; i <= m; i = i + s)
                {
                    m_Selection[i] = true;
                }
            }
            else
            {
                if (m_Selection.size() < n + 1)
                {
                    m_Selection.resize(n + 1);
                }
                m_Selection[n] = true;
                m_UnlimitedRules.push_back(std::make_pair(n, s));
            }
        }
    }

    // process unlimited selections to fill up existing selection size
    for (auto u : m_UnlimitedRules)
    {
        for (size_t i = u.first; i < m_Selection.size(); i = i + u.second)
        {
            m_Selection[i] = true;
        }
    }
}

bool RangeFilter::IsSelected(size_t n)
{
    size_t selSize = m_Selection.size();
    if (selSize == 0 && m_UnlimitedRules.size() == 0)
    {
        // uninitialized filter returns true for everything
        return true;
    }
    if (m_Selection.size() > n)
    {
        return m_Selection[n];
    }
    else
    {
        for (auto u : m_UnlimitedRules)
        {
            size_t k =
                n - u.first; // > 0 beacuse u.first was placed in m_Selection
            if (k % u.second == 0)
            {
                return true;
            }
        }
        return false;
    }
}

size_t RangeFilter::ToSizeT(const std::string &input)
{
    try
    {
        size_t pos;
        const size_t out = static_cast<size_t>(std::stoul(input, &pos));
        if (pos < input.size())
        {
            std::throw_with_nested(std::invalid_argument(
                "ERROR: could not cast string '" + input + "' to number "));
        }
        return out;
    }
    catch (...)
    {
        std::throw_with_nested(std::invalid_argument(
            "ERROR: could not cast string '" + input + "' to number "));
    }
}

} // end namespace helper
} // end namespace adios2
