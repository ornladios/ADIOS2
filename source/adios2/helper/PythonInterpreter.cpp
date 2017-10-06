/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * PythonInterpreter.cpp
 *
 *  Created on: Sept 27, 2017
 *      Author: Scott Wittenburg <scott.wittenburg@kitware.com>
 */

#include "PythonInterpreter.h"

#include <pybind11/embed.h>

#if WIN32
#include <Python.h>
extern __declspec(dllimport) int Py_NoSiteFlag;
#endif

namespace adios2
{

PythonInterpreter PythonInterpreter::m_instance;

PythonInterpreter& PythonInterpreter::instance()
{
  return PythonInterpreter::m_instance;
}

PythonInterpreter::PythonInterpreter()
  : m_embedded(false)
{
  this->initialize();
}

PythonInterpreter::~PythonInterpreter()
{
  this->finalize();
}

bool PythonInterpreter::isInitialized() const
{
  return Py_IsInitialized() != 0;
}

void PythonInterpreter::initialize()
{
  if (this->isInitialized())
  {
    return;
  }

  m_embedded = true;
  Py_NoSiteFlag = 1;
  pybind11::initialize_interpreter();
}

void PythonInterpreter::finalize()
{
  if (this->isInitialized())
  {
    pybind11::finalize_interpreter();
    m_embedded = false;
  }
}
}