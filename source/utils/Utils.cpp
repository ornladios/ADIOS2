/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Utils.h"

namespace adios2
{

Utils::Utils(const std::string name, int argc, char *argv[])
: m_Name(name), m_Arguments(argv, argv + argc)
{
}

} // end namespace adios2
