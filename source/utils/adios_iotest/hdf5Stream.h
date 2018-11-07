/*
 * hdf5Stream.h
 *
 *  Created on: Nov 2018
 *      Author: Norbert Podhorszki
 */

#ifndef HDF5STREAM_H
#define HDF5STREAM_H

#include "hdf5.h"
#include "stream.h"

#include <map>
#include <string>

struct hdf5VarInfo
{
    hid_t dataspace;
    hid_t dataset;
    hdf5VarInfo(hid_t dataset, hid_t dataspace)
    : dataset(dataset), dataspace(dataspace){};
};

using H5VarMap = std::map<std::string, hdf5VarInfo>;

class hdf5Stream : public Stream
{
public:
    hid_t h5file;
    H5VarMap varmap;
    hdf5Stream(const std::string &streamName, const adios2::Mode mode,
               MPI_Comm comm);
    ~hdf5Stream();
    void Write(CommandWrite *cmdW, Config &cfg, const Settings &settings,
               size_t step);
    adios2::StepStatus Read(CommandRead *cmdR, Config &cfg,
                            const Settings &settings, size_t step);
    void Close();

private:
    hid_t hdf5Type(std::string &type);
    int nSteps = 0;
    void defineHDF5Array(const std::shared_ptr<VariableInfo> ov);
    void putHDF5Array(const std::shared_ptr<VariableInfo> ov, size_t step);
    void getHDF5Array(std::shared_ptr<VariableInfo> ov, size_t step);
};

#endif /* HDF5STREAM_H */
