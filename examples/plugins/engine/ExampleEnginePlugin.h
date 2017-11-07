/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ExampleEnginePlugin.h This plugin does nothing but write API calls out to a
 * log file.
 *
 *  Created on: Jul 31, 2017
 *      Author: Chuck Atkins <chuck.atkins@kitware.com>
 */

#ifndef EXAMPLEENGINEPLUGIN_H_
#define EXAMPLEENGINEPLUGIN_H_

#include <fstream>
#include <string>

#include <adios2/ADIOSMPICommOnly.h>
#include <adios2/ADIOSMacros.h>
#include <adios2/ADIOSTypes.h>
#include <adios2/core/IO.h>
#include <adios2/engine/plugin/PluginEngineInterface.h>

namespace adios2
{

/** An engine interface to be used aby the plugin infrastructure */
class ExampleEnginePlugin : public PluginEngineInterface
{
public:
    ExampleEnginePlugin(IO &io, const std::string &name, const Mode openMode,
                        MPI_Comm mpiComm);
    virtual ~ExampleEnginePlugin();

    void Close(const int transportIndex = -1) override;

protected:
    void Init() override;

#define declare(T)                                                             \
    void DoPutSync(Variable<T> &variable, const T *values) override;
    ADIOS2_FOREACH_TYPE_1ARG(declare)
#undef declare

private:
    std::ofstream m_Log;
};

} // end namespace adios2
#endif /* EXAMPLEENGINEPLUGIN_H_ */
