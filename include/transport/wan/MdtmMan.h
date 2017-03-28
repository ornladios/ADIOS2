/*
 * MdtmMan.h
 *
 *  Created on: Jan 18, 2017
 *      Author: wfg
 */

#ifndef MDTMMAN_H_
#define MDTMMAN_H_

#include "core/Transport.h"
#include "external/json.hpp"

#include "DataMan.h" //here comes your DataMan header

namespace adios
{
namespace transport
{

class MdtmMan : public Transport
{

public:
  /**
   *
   * @param localIP
   * @param remoteIP
   * @param mode
   * @param prefix
   * @param numberOfPipes
   * @param tolerances
   * @param priorities
   * @param mpiComm
   * @param debugMode
   */
  MdtmMan(const std::string localIP, const std::string remoteIP,
          const std::string mode, const std::string prefix,
          const int numberOfPipes, const std::vector<int> tolerances,
          const std::vector<int> priorities, MPI_Comm mpiComm,
          const bool debugMode);

  ~MdtmMan();

  void Open(const std::string name, const std::string accessMode);

  void SetBuffer(char *buffer, std::size_t size);

  /**
   * We can always overload this function in the base class and accept other
   * types of data pointers, e.g. Write( json* );
   * I'm sticking with char* as it's more general (only C++ libraries, e.g.
   * boost understand std::std::vector, MPI, POSIX, Infiniband use pointer*)
   * @param buffer
   * @param size
   */
  void Write(const char *buffer, std::size_t size);

  void Flush(); ///< not sure if this one is needed...

  void Close();

private:
  std::string m_LocalIP;    ///< local ip address, can change over time
  std::string m_RemoteIP;   ///<  remote ip address, can change over time
  std::string m_Mode;       ///< send/write, receive/read
  std::string m_Prefix;     ///< prefix given to message
  int m_NumberOfPipes = -1; ///< should it be unsigned int?
  std::vector<int> m_Tolerances;
  std::vector<int> m_Priorities;

  /**
   * Should we change data to char* ?
   * @param data
   * @param doid
   * @param variable
   * @param dType
   * @param putShape
   * @param varShape
   * @param offset
   * @param timestep
   * @param tolerance
   * @param priority
   * @return
   */
  int Put(const void *data, const std::string doid, const std::string variable,
          const std::string dType, const std::vector<std::uint64_t> &putShape,
          const std::vector<uint64_t> &varShape,
          const std::vector<uint64_t> &offset, const std::uint64_t timestep,
          const int tolerance, const int priority);

  /**
   *
   * @param data
   * @param doid
   * @param variable
   * @param dType
   * @param putShape
   * @param varShape
   * @param offset
   * @param timestep
   * @param tolerance
   * @param priority
   * @return
   */
  int Get(void *data, const std::string doid, const std::string variable,
          const std::string dType, const std::vector<std::uint64_t> &putShape,
          const std::vector<uint64_t> &varShape,
          const std::vector<uint64_t> &offset, const std::uint64_t timestep,
          const int tolerance, const int priority);

  /**
   *
   * @param data
   * @param doid
   * @param variable
   * @param dType
   * @param varShape
   * @param timestep
   * @return
   */
  int Get(void *data, const std::string doid, const std::string variable,
          const std::string dType, std::vector<std::uint64_t> &varShape,
          const std::uint64_t timestep);

  /**
   *
   * @param jData
   */
  void OnReceive(nlohmann::json &jData);
};

} // end namespace transport
} // end namespace

#endif /* MDTMMAN_H_ */
