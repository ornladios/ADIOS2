/*
 * adiosPyFunctions.cpp
 *
 *  Created on: Mar 13, 2017
 *      Author: wfg
 */
#include <iostream>

#include "adiosPyFunctions.h"

namespace adios
{

#ifdef HAVE_BOOSTPYTHON
namespace py = boost::python;
#endif

#ifdef HAVE_PYBIND11
namespace py = pybind11;
#endif

Dims ListToVector(const pyList &list)
{
  const unsigned int length = py::len(list);
  Dims vec;
  vec.reserve(length);

  for (unsigned int i = 0; i < length; i++)
    vec.push_back(PyCast<std::size_t>(list[i]));

  return vec;
}

#ifdef HAVE_BOOSTPYTHON
std::map<std::string, std::string> DictToMap(const pyDict &dictionary)
{
  std::map<std::string, std::string> parameters;

  pyList keys = dictionary.keys();
  const unsigned int length = py::len(keys);

  for (unsigned int k = 0; k < length; ++k)
  {
    const std::string key(PyCast<std::string>(keys[k]));
    const std::string value(PyCast<std::string>(dictionary[keys[k]]));
    parameters.insert(std::make_pair(key, value));
  }

  return parameters;
}
#endif

#ifdef HAVE_PYBIND11
std::map<std::string, std::string> KwargsToMap(const pybind11::kwargs &kwargs)
{
  std::map<std::string, std::string> parameters;

  for (const auto &pair : kwargs)
  {
    const std::string key(PyCast<std::string>(pair.first));
    const std::string value(PyCast<std::string>(pair.second));
    parameters.insert(std::make_pair(key, value));
  }
  return parameters;
}
#endif

bool IsEmpty(pyObject object)
{
  bool isEmpty = false;

#ifdef HAVE_BOOSTPYTHON
  if (object == boost::python::object())
    isEmpty = true;
#endif

#ifdef HAVE_PYBIND11
  if (object == pybind11::none())
    isEmpty = true;
#endif
  return isEmpty;
}

} // end namespace
