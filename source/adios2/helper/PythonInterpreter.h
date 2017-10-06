/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * PythonInterpreter.h Singleton python interpreter functionality
 *
 *  Created on: Sept 27, 2017
 *      Author: Scott Wittenburg <scott.wittenburg@kitware.com>
 */

#ifndef ADIOS2_HELPER_PYTHONINTERPRETER_H_
#define ADIOS2_HELPER_PYTHONINTERPRETER_H_

namespace adios2
{

/** @brief Singleton class for encapsulating the embedded interpreter.
           Ensures that the interpreter is initialized/finalized at static
           initialization/deinitialization.  It is left to the user to ensure
           that their python modules can be found on the python path.
  */
class PythonInterpreter
{
public:
  static PythonInterpreter& instance();

  // Check if python is initialized.
  bool isInitialized() const;

  // Initialize the embedded python
  void initialize();

  // Finalize the embedded python.
  void finalize();

  // Returns true if the embedded python session has been initialized.
  bool isEmbedded() const { return m_embedded; }

private:
  PythonInterpreter();
  virtual ~PythonInterpreter();

  static PythonInterpreter m_instance;

  bool m_embedded;
};
}

#endif /* ADIOS2_HELPER_PYTHONINTERPRETER_H_ */