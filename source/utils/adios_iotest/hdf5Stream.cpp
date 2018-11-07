/*
 * hdf5Stream.cpp
 *
 *  Created on: Nov 2018
 *      Author: Norbert Podhorszki
 */

#include "hdf5Stream.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <math.h>
#include <stdexcept>
#include <string>

hdf5Stream::hdf5Stream(const std::string &streamName, const adios2::Mode mode,
                       MPI_Comm comm)
: Stream(streamName, mode)
{
    hid_t acc_tpl = H5Pcreate(H5P_FILE_ACCESS);
    MPI_Info info = MPI_INFO_NULL;
    herr_t ret = H5Pset_fapl_mpio(acc_tpl, comm, info);

    if (mode == adios2::Mode::Write)
    {
        h5file =
            H5Fcreate(streamName.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, acc_tpl);
    }
    else
    {
        h5file = H5Fopen(streamName.c_str(), H5F_ACC_RDONLY, acc_tpl);
    }
    ret = H5Pclose(acc_tpl);
}

hdf5Stream::~hdf5Stream(){};

hid_t hdf5Stream::hdf5Type(std::string &type)
{
    hid_t htype = H5T_NATIVE_CHAR;
    if (type == "double")
    {
        htype = H5T_NATIVE_DOUBLE;
    }
    else if (type == "float")
    {
        htype = H5T_NATIVE_FLOAT;
    }
    else if (type == "int")
    {
        htype = H5T_NATIVE_INT32;
    }
    return htype;
}

void hdf5Stream::defineHDF5Array(const std::shared_ptr<VariableInfo> ov)
{
    const int ndim = ov->ndim + 1;
    hsize_t maxdims[ndim];
    hsize_t dims[ndim];
    hsize_t count[ndim];

    maxdims[0] = H5S_UNLIMITED;
    dims[0] = 1;
    count[0] = 1;
    for (int d = 1; d < ndim; ++d)
    {
        maxdims[d] = ov->shape[d - 1];
        dims[d] = ov->shape[d - 1];
        count[d] = ov->count[d - 1];
    }

    hid_t dataspace = H5Screate_simple(ndim, dims, maxdims);

    hid_t cparms;
    /*
     * Set chunking, the only way to have extendible arrays
     */
    cparms = H5Pcreate(H5P_DATASET_CREATE);
    H5Pset_chunk(cparms, ndim, count);

    hid_t dataset;
    dataset = H5Dcreate2(h5file, ov->name.c_str(), hdf5Type(ov->type),
                         dataspace, H5P_DEFAULT, cparms, H5P_DEFAULT);
    varmap.emplace(std::make_pair(ov->name, hdf5VarInfo(dataset, dataspace)));
    H5Pclose(cparms);
}

void hdf5Stream::putHDF5Array(const std::shared_ptr<VariableInfo> ov,
                              size_t step)
{
    /* note: step starts from 1 */
    const auto it = varmap.find(ov->name);
    hdf5VarInfo &vi = it->second;
    int ndim = ov->ndim + 1;
    hsize_t start[ndim];
    hsize_t count[ndim];
    hsize_t dims[ndim];
    start[0] = step - 1;
    count[0] = 1;
    dims[0] = step;
    for (int d = 1; d < ndim; ++d)
    {
        start[d] = ov->start[d - 1];
        count[d] = ov->count[d - 1];
        dims[d] = ov->shape[d - 1];
    }

    H5Dset_extent(vi.dataset, dims);
    hid_t filespace = H5Dget_space(vi.dataset);
    H5Sselect_hyperslab(filespace, H5S_SELECT_SET, start, NULL, count, NULL);
    hid_t dxpl_id = H5Pcreate(H5P_DATASET_XFER);
    H5Pset_dxpl_mpio(dxpl_id, H5FD_MPIO_COLLECTIVE);
    hid_t memspace = H5Screate_simple(ndim, count, NULL);
    H5Dwrite(vi.dataset, hdf5Type(ov->type), memspace, filespace, dxpl_id,
             ov->data.data());
    H5Pclose(dxpl_id);
    H5Sclose(filespace);
    H5Sclose(memspace);
}
void hdf5Stream::Write(CommandWrite *cmdW, Config &cfg,
                       const Settings &settings, size_t step)
{
    if (!settings.myRank && settings.verbose)
    {
        std::cout << "    Write to HDF5 output " << cmdW->streamName
                  << " the group " << cmdW->groupName;
        if (!cmdW->variables.empty())
        {
            std::cout << " with selected variables:  ";
            for (const auto &v : cmdW->variables)
            {
                std::cout << v->name << " ";
            }
        }
        std::cout << std::endl;
    }

    const double div =
        pow(10.0, static_cast<const double>(settings.ndigits(cfg.nSteps - 1)));
    double myValue = static_cast<double>(settings.myRank) +
                     static_cast<double>(step - 1) / div;

    for (auto ov : cmdW->variables)
    {
        if (step == 1)
        {
            if (!settings.myRank && settings.verbose)
            {
                std::cout << "        Define array  " << ov->name
                          << "  for output" << std::endl;
            }
            defineHDF5Array(ov);
        }

        // if we read the variable, use the read values otherwise generate data
        // now
        if (!ov->readFromInput)
        {
            if (!settings.myRank && settings.verbose)
            {
                std::cout << "        Fill array  " << ov->name
                          << "  for output" << std::endl;
            }
            fillArray(ov, myValue);
        }
    }

    if (!settings.myRank && settings.verbose)
    {
        std::cout << "        Write data " << std::endl;
    }
    for (const auto ov : cmdW->variables)
    {
        putHDF5Array(ov, step);
    }
}

void hdf5Stream::getHDF5Array(std::shared_ptr<VariableInfo> ov, size_t step)
{
    hid_t dataset = H5Dopen2(h5file, ov->name.c_str(), H5P_DEFAULT);
    if (dataset == -1)
    {
        std::cout << "        Variable " << ov->name
                  << " is not in the file: " << std::endl;
        ov->readFromInput = false;
        return;
    }
    hid_t filespace = H5Dget_space(dataset);
    // hsize_t dims[ov->ndim + 1];
    int ndim = H5Sget_simple_extent_ndims(filespace);
    // H5Sget_simple_extent_dims(filespace, dims, NULL);
    /* ndim == var->ndim+1 */

    hsize_t start[ndim];
    hsize_t count[ndim];
    start[0] = step - 1;
    count[0] = 1;
    for (int d = 1; d < ndim; ++d)
    {
        start[d] = ov->start[d - 1];
        count[d] = ov->count[d - 1];
    }

    hid_t memspace = H5Screate_simple(ndim, count, NULL);
    hid_t dxpl_id = H5Pcreate(H5P_DATASET_XFER);
    H5Pset_dxpl_mpio(dxpl_id, H5FD_MPIO_COLLECTIVE);
    H5Sselect_hyperslab(filespace, H5S_SELECT_SET, start, NULL, count, NULL);
    void *buf = reinterpret_cast<void *>(ov->data.data());
    H5Dread(dataset, hdf5Type(ov->type), memspace, filespace, dxpl_id, buf);

    H5Sclose(memspace);
    H5Sclose(filespace);
    H5Pclose(dxpl_id);
    H5Dclose(dataset);
    ov->readFromInput = true;
}

adios2::StepStatus hdf5Stream::Read(CommandRead *cmdR, Config &cfg,
                                    const Settings &settings, size_t step)
{
    if (!settings.myRank && settings.verbose)
    {
        std::cout << "    Read ";
        if (cmdR->stepMode == adios2::StepMode::NextAvailable)
        {
            std::cout << "next available step from ";
        }
        else
        {
            std::cout << "latest step from ";
        }

        std::cout << cmdR->streamName << " using the group " << cmdR->groupName;
        if (!cmdR->variables.empty())
        {
            std::cout << " with selected variables:  ";
            for (const auto &v : cmdR->variables)
            {
                std::cout << v->name << " ";
            }
        }
        std::cout << std::endl;
    }

    if (step == 1)
    {
        /* Get the number of available steps in the HDF5 file */
        std::shared_ptr<VariableInfo> var = cmdR->variables.front();
        hid_t dataset = H5Dopen2(h5file, var->name.c_str(), H5P_DEFAULT);
        hid_t filespace = H5Dget_space(dataset);
        hsize_t dims[var->ndim + 1];
        int ndim = H5Sget_simple_extent_ndims(filespace);
        H5Sget_simple_extent_dims(filespace, dims, NULL);
        /* ndim == var->ndim+1 */
        nSteps = dims[0];
        if (!settings.myRank && settings.verbose)
        {
            std::cout << "        Number of steps in file: " << nSteps
                      << std::endl;
        }
        H5Sclose(filespace);
        H5Dclose(dataset);
    }

    if (step > nSteps)
    {
        return adios2::StepStatus::EndOfStream;
    }

    /*
    if (!settings.myRank && settings.verbose && step == 1)
    {
        const auto varmap = PARSE HERE THE HDF5 FILE FOR VARIABLES
        std::cout << "    Variables in input for reading: " << std::endl;
        for (const auto &v : varmap)
        {
            std::cout << "        " << v.first << std::endl;
        }
    }
    */

    if (!settings.myRank && settings.verbose)
    {
        std::cout << "    Read data " << std::endl;
    }

    for (auto ov : cmdR->variables)
    {
        getHDF5Array(ov, step);
    }

    return adios2::StepStatus::OK;
}
void hdf5Stream::Close()
{
    for (const auto it : varmap)
    {
        auto &vi = it.second;
        H5Dclose(vi.dataset);
        H5Sclose(vi.dataspace);
    }
    H5Fclose(h5file);
}
