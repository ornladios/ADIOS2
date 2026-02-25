/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ExampleWritePlugin.tcc
 *
 *  Created on: Jul 5, 2021
 *      Author: Chuck Atkins <chuck.atkins@kitware.com>
 *              Caitlin Ross <caitlin.ross@kitware.com>
 */

#ifndef EXAMPLEWRITEPLUGIN_TCC_
#define EXAMPLEWRITEPLUGIN_TCC_

#include "ExampleWritePlugin.h"

namespace adios2
{
namespace plugin
{

template <typename T>
void ExampleWritePlugin::WriteVariableInfo(adios2::Variable<T> variable)
{
    /** write basic variable info to file **/
    m_VarFile << variable.Name() << ";" << variable.Type() << ";" << variable.Shape() << ";"
              << variable.Start() << ";" << variable.Count() << std::endl;
}

template <typename T>
void ExampleWritePlugin::WriteArray(adios2::Variable<T> variable, const T *values)
{
    /** Write variable name and step to file, followed by the actual data on the
     * next line **/
    m_DataFile << variable.Name() << "," << m_CurrentStep << std::endl;
    auto selSize = variable.SelectionSize();
    for (size_t i = 0; i < selSize; ++i)
    {
        m_DataFile << values[i];
        if (i < selSize - 1)
        {
            m_DataFile << ",";
        }
        else
        {
            m_DataFile << std::endl;
        }
    }
}

} // end namespace plugin
} // end namespace adios2
#endif /* EXAMPLEWRITEPLUGIN_TCC_ */
