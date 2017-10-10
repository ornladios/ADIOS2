/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * PythonModuleHelper.h Helper method to find a python class, importing
 *                      it if necessary.
 *
 *  Created on: Oct 02, 2017
 *      Author: Scott Wittenburg <scott.wittenburg@kitware.com>
 */

#ifndef ADIOS2_HELPER_PYTHONMODULEHELPER_H_
#define ADIOS2_HELPER_PYTHONMODULEHELPER_H_

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace adios2
{
namespace PythonModuleHelper
{
    pybind11::object FindPythonClass(const std::string& className,
                                     const std::string* moduleName = nullptr)
    {
        pybind11::dict globals = pybind11::globals();

        if (moduleName != nullptr)
        {
            pybind11::object moduleObject;

            if (globals.contains((*moduleName).c_str()))
            {
                moduleObject = globals[(*moduleName).c_str()];
            }
            else
            {
                moduleObject = pybind11::module::import((*moduleName).c_str());
            }

            pybind11::object constructor =
                moduleObject.attr(className.c_str());
            return constructor;
        }
        else if (globals.contains(className.c_str()))
        {
            pybind11::object constructor = globals[className.c_str()];
            return constructor;
        }

        // Unable to instantiate the object in this case
        throw std::runtime_error("PythonInstanceBuilder: Specified class was "
                                 "not present in main module, nor was a "
                                 "module name provided in the parameters.  "
                                 "Unable to instantiate python class.");
    }
}
}

#endif /* ADIOS2_HELPER_PYTHONMODULEHELPER_H_ */
