/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataSpacesWriter.cpp
 *
 *  Created on: Dec 5, 2018
 *      Author: Pradeep Subedi
 *      		pradeep.subedi@rutgers.edu
 */

#include <memory>

#include "DataSpacesWriter.h"
#include "DataSpacesWriter.tcc"
#include "adios2/toolkit/dataspaces/ds_data.h"
#include "adios2/helper/adiosFunctions.h" //CSVToVector
#include "dataspaces.h"

namespace adios2
{
namespace core
{
namespace engine
{

DataSpacesWriter::DataSpacesWriter(IO &io, const std::string &name, const Mode mode,
                     MPI_Comm mpiComm)
: Engine("DataSpacesWriter", io, name, mode, mpiComm)
{


	f_Name=name;
    int ret = 0;
    auto appID = m_IO.m_Parameters.find("AppID");
    if (appID != m_IO.m_Parameters.end()){
    	m_data.appid = std::stoi(appID->second);
	}else{
		m_data.appid = 0;

    }
    ret = adios_dataspaces_init(&mpiComm, &m_data);
    if(ret< 0)
    	fprintf(stderr, "Unable to connect to DataSpaces. Err: %d\n", ret);

}
DataSpacesWriter::~DataSpacesWriter(){ }

StepStatus DataSpacesWriter::BeginStep(StepMode mode, const float timeout_sec)
{
	//acquire lock in Begin Step
	m_CurrentStep++; // current step begins at 0;
	/*
	std::string l_Name= f_Name + std::to_string(m_CurrentStep);
	char *cstr = new char[l_Name.length() + 1];
	strcpy(cstr, l_Name.c_str());

	dspaces_lock_on_write (cstr, &m_data.mpi_comm);
	delete[] cstr;
	*/
    return StepStatus::OK;
}

size_t DataSpacesWriter::CurrentStep() const
{
    return m_CurrentStep;
}

void DataSpacesWriter::EndStep()
{
	int rank;
	MPI_Comm_rank(m_data.mpi_comm, &rank);
	WriteVarInfo();
	MPI_Barrier(m_data.mpi_comm);
}
void DataSpacesWriter::Flush(const int transportIndex) {}

void DataSpacesWriter::DoClose(const int transportIndex){
	// disconnect from dataspaces if we are connected from writer but not anymore from reader
	std::string local_file_var = f_Name + std::to_string(m_CurrentStep+1);
	char *meta_lk = new char[local_file_var.length() + 1];
	strcpy(meta_lk, local_file_var.c_str());

	dspaces_lock_on_write (meta_lk, &(m_data.mpi_comm));
	dspaces_unlock_on_write (meta_lk, &(m_data.mpi_comm));

	if (globals_adios_is_dataspaces_connected_from_writer() &&
			!globals_adios_is_dataspaces_connected_from_both())
	{
		//fprintf(stderr, "Disconnecting writer via finalize \n");
		MPI_Barrier (m_data.mpi_comm);
		dspaces_finalize();

	}
	globals_adios_set_dataspaces_disconnected_from_writer();
}

void DataSpacesWriter::PerformPuts() {}

#define declare_type(T)                                                        \
    void DataSpacesWriter::DoPutSync(Variable<T> &variable, const T *values)        \
    {                                                                          \
        DoPutSyncCommon(variable, values);                                     \
    }                                                                          \
    void DataSpacesWriter::DoPutDeferred(Variable<T> &variable, const T *values)    \
    {                                                                          \
        DoPutSyncCommon(variable, values);                                     \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void DataSpacesWriter::WriteVarInfo()
{

	std::string local_file_var;
	int elemsize, ndim, nvars, rank;
	int *elemSize_meta, *dim_meta;
	char * local_str, *buffer, *name_string;
	uint64_t *gdim_meta;
	uint64_t gdims[MAX_DS_NDIM], lb[MAX_DS_NDIM], ub[MAX_DS_NDIM];

	local_file_var = f_Name + std::to_string(m_CurrentStep);
	char *meta_lk = new char[local_file_var.length() + 1];
	strcpy(meta_lk, local_file_var.c_str());
	MPI_Comm_rank(m_data.mpi_comm, &rank);

	dspaces_lock_on_write (meta_lk, &(m_data.mpi_comm));
	if(rank==0){

		std::string ds_file_var;
		int var_num = ndim_vector.size();
		int var_name_max_length = 128;
		int buf_len = sizeof(int)+ var_num * sizeof(int) +var_num * sizeof(int)+ MAX_DS_NDIM * var_num * sizeof(uint64_t) + var_num * var_name_max_length * sizeof(char);
		int *dim_meta, *elemSize_meta;
		uint64_t *gdim_meta;
		int* nvar_buf;
		nvar_buf = (int*)malloc(sizeof(int));
		dim_meta = (int*) malloc(var_num* sizeof(int));
		elemSize_meta = (int*) malloc(var_num*sizeof(int));
		gdim_meta = (uint64_t *)malloc(MAX_DS_NDIM * var_num * sizeof(uint64_t));
		memset(gdim_meta, 0, MAX_DS_NDIM * var_num * sizeof(uint64_t));
		buffer = (char*) malloc(buf_len);
		name_string= (char*) malloc(var_num * var_name_max_length * sizeof(char));
		nvar_buf[0] = var_num;
		for (nvars = 0; nvars < var_num; ++nvars) {
			char *cstr = new char[v_name_vector.at(nvars).length() + 1];
			strcpy(cstr, v_name_vector.at(nvars).c_str());
			//copy the name to specific offset
			memcpy( &name_string[nvars*var_name_max_length], &cstr[0], (v_name_vector.at(nvars).length() + 1)*sizeof( char ) );

			dim_meta[nvars] = ndim_vector[nvars]; //store the ndim information for each variable
			elemSize_meta[nvars] = elemSize_vector[nvars];
			for (int i = 0; i < dim_meta[nvars]; i++)
			{
				gdim_meta[nvars*MAX_DS_NDIM+i] = gdims_vector[nvars].at(i);
			}


		}
		//copy all the data into payload buffer
		memcpy(buffer, nvar_buf, sizeof(int));
		memcpy(&buffer[sizeof(int)], dim_meta, var_num* sizeof(int));
		memcpy(&buffer[var_num* sizeof(int)], elemSize_meta, var_num* sizeof(int));
		memcpy(&buffer[2*var_num* sizeof(int)], gdim_meta, MAX_DS_NDIM * var_num * sizeof(uint64_t));
		memcpy(&buffer[2*var_num* sizeof(int)+MAX_DS_NDIM * var_num * sizeof(uint64_t)], name_string, var_num * var_name_max_length * sizeof(char));

		//store metadata in DataSoaces
		local_file_var = "VARMETA@"+f_Name;
		local_str = new char[local_file_var.length() + 1];
		strcpy(local_str, local_file_var.c_str());

		elemsize = sizeof(char);
		ndim = 1;
		lb[0] = 0; ub[0] = buf_len-1;
		gdims[0] = (ub[0]-lb[0]+1) * dspaces_get_num_space_server();
		dspaces_define_gdim(local_str, ndim, gdims);

		dspaces_put(local_str, m_CurrentStep, elemsize, ndim, lb, ub, buffer);
		//delete[] local_str;


		/*
		memset(lb, 0, MAX_DS_NDIM * sizeof(uint64_t));
		memset(ub, 0, MAX_DS_NDIM * sizeof(uint64_t));
		memset(gdims, 0, MAX_DS_NDIM * sizeof(uint64_t));
		//store the latest version or step information for the file and how many variables are there in the file

		int version_buf[2] = {var_num,0}; // Put var_numbers in each step in DataSpaces
		int version_buf_len = 2;
		local_file_var = "VERSION@"+f_Name;
		local_str = new char[local_file_var.length() + 1];
		strcpy(local_str, local_file_var.c_str());
		elemsize = sizeof(int);
		ndim = 1;
		lb[0] = 0; ub[0] = version_buf_len-1;
		gdims[0] = (ub[0]-lb[0]+1) * dspaces_get_num_space_server();
		dspaces_define_gdim(local_str, ndim, gdims);

		dspaces_put(local_str, m_CurrentStep, elemsize, ndim, lb, ub, version_buf);
		delete[] local_str;

		int l_version_buf[2] = {m_CurrentStep,0}; // Put the latest version number to dataspaces
		local_file_var = "LATESTVERSION@"+f_Name;
		local_str = new char[local_file_var.length() + 1];
		strcpy(local_str, local_file_var.c_str());
		dspaces_define_gdim(local_str, ndim, gdims);

		dspaces_put(local_str, 0, elemsize, ndim, lb, ub, l_version_buf);
		*/
		dspaces_put_sync(); //wait on previous put to finish
		delete[] local_str;
		free(dim_meta);
		free(elemSize_meta);
		free(gdim_meta);
		free(buffer);
		free(name_string);
		free(nvar_buf);
	}
	dspaces_unlock_on_write (meta_lk, &(m_data.mpi_comm));
	ndim_vector.clear();
	gdims_vector.clear();
	v_name_vector.clear();
	elemSize_vector.clear();

}



} // end namespace engine
} // end namespace core
} // end namespace adios2



