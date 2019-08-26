/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * NullEngine.h : Null engine does nothing, used for benchmarking
 *
 *  Created on: Mar 11, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_ENGINE_NULL_NULLENGINE_H_
#define ADIOS2_ENGINE_NULL_NULLENGINE_H_

#include "adios2/common/ADIOSConfig.h"
#include "adios2/core/Engine.h"
#include "adios2/helper/adiosComm.h"

namespace adios2
{
namespace core
{
namespace engine
{

class NullEngine : public Engine
{
public:
    NullEngine(IO &adios, const std::string &name, const Mode mode,
               helper::Comm comm);

    ~NullEngine() = default;

private:
    void DoClose(const int transportIndex = -1);
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_NULL_NULLREADER_H_ */
