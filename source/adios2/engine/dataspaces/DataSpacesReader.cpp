/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataSpacesReader.cpp
 *
 *  Created on: Dec 5, 2018
 *      Author: Pradeep Subedi
 *      		pradeep.subedi@rutgers.edu
 */
#include <memory>

#include "DataSpacesReader.h"
#include "DataSpacesReader.tcc"
#include "adios2/toolkit/dataspaces/ds_data.h"
#include "adios2/helper/adiosFunctions.h" //CSVToVector
#include "dataspaces.h"

namespace adios2
{
namespace core
{
namespace engine
{

DataSpacesReader::DataSpacesReader(IO &io, const std::string &name, const Mode mode,
                     MPI_Comm mpiComm)
: Engine("DataSpacesReader", io, name, mode, mpiComm)
{


	f_Name=name;
    int ret = 0;
    latestStep = 0;
    nVars = 0;
    m_CurrentStep=-1;
    ret = adios_read_dataspaces_init(&mpiComm, &m_data);
    if(ret < 0){
    	fprintf(stderr, "DataSpaces did not initialize properly %d\n", ret);
    }

}

DataSpacesReader::~DataSpacesReader() { DoClose(); }


StepStatus DataSpacesReader::BeginStep(StepMode mode, const float timeout_sec)
{

	std::string ds_file_var;
	std::string local_file_var;
	uint64_t gdims[MAX_DS_NDIM], lb[MAX_DS_NDIM], ub[MAX_DS_NDIM];
	int elemsize, ndim;
	//acquire lock, current step and n_vars in the Begin Step
	char *cstr = new char[f_Name.length() + 1];
	strcpy(cstr, f_Name.c_str());

	fprintf (stderr, "rank=%d call read lock...\n", m_data.rank);
	dspaces_lock_on_read (cstr, &m_data.mpi_comm);
	fprintf (stderr, "rank=%d got read lock\n", m_data.rank);

	delete[] cstr;

	//read the version and nvars from dataspaces
	int l_version_no[2] = {0,0};
	int version_buf_len = 2;
	local_file_var = "LATESTVERSION@"+f_Name;
	cstr = new char[local_file_var.length() + 1];
	strcpy(cstr, local_file_var.c_str());
	elemsize = sizeof(int);
	ndim = 1;
	lb[0] = 0; ub[0] = version_buf_len-1;
	gdims[0] = (ub[0]-lb[0]+1) * dspaces_get_num_space_server();
	dspaces_define_gdim(cstr, ndim, gdims);
	dspaces_get(cstr, 0, elemsize, ndim, lb, ub, l_version_no);
	delete[] cstr;

	latestStep = l_version_no[0];
	//fprintf(stderr,"LatestStep Read from DSpaces %d\n", latestStep);

	if(mode == StepMode::NextAvailable){
		m_CurrentStep++;
	}
	if(mode == StepMode::LatestAvailable){
			m_CurrentStep = latestStep;
	}
	//we check if the current step is the end of the step that is in DataSpaces
	if (m_CurrentStep > latestStep)
	{
		return StepStatus::EndOfStream;
	}

	int version_buf[2] = {0,0}; /* last version put in space; not terminated */
	local_file_var = "VERSION@"+f_Name;
	cstr = new char[local_file_var.length() + 1];
	strcpy(cstr, local_file_var.c_str());
	dspaces_define_gdim(cstr, ndim, gdims);
	dspaces_get(cstr, 0, elemsize, ndim, lb, ub, version_buf);

	delete[] cstr;
	nVars = version_buf[0];

	m_IO.RemoveAllVariables();

	memset(lb, 0, MAX_DS_NDIM * sizeof(uint64_t));
	memset(ub, 0, MAX_DS_NDIM * sizeof(uint64_t));
	memset(gdims, 0, MAX_DS_NDIM * sizeof(uint64_t));

	//now populate vars in the IO
	int var_name_max_length = 128;
	int buf_len = nVars * sizeof(int) + nVars * sizeof(int)+ MAX_DS_NDIM * nVars * sizeof(uint64_t) + nVars * var_name_max_length * sizeof(char);
	char *buffer;
	buffer = (char*) malloc(buf_len);
	memset(buffer, 0, buf_len);
	//fprintf(stderr, "Num Vars: %d, BufLen: %d\n", nVars, buf_len);
	char * local_str;
	local_file_var = "VARMETA@"+f_Name;
	local_str = new char[local_file_var.length() + 1];
	strcpy(local_str, local_file_var.c_str());

	elemsize = sizeof(char);
	ndim = 1;
	lb[0] = 0; ub[0] = buf_len-1;
	gdims[0] = (ub[0]-lb[0]+1) * dspaces_get_num_space_server();
	dspaces_define_gdim(local_str, ndim, gdims);

	dspaces_get(local_str, latestStep, elemsize, ndim, lb, ub, buffer);

	//now populate data from the buffer
	//fprintf(stderr, "After receipt of long buffer\n");

	int *dim_meta;
	dim_meta = (int*) malloc(nVars* sizeof(int));
	int *elemSize_meta;
	elemSize_meta = (int*) malloc(nVars*sizeof(int));
	uint64_t *gdim_meta;
	gdim_meta = (uint64_t *)malloc(MAX_DS_NDIM * nVars * sizeof(uint64_t));
	memset(gdim_meta, 0, MAX_DS_NDIM * nVars * sizeof(uint64_t));

	memcpy(dim_meta, buffer,  nVars* sizeof(int));
	memcpy(elemSize_meta, &buffer[nVars* sizeof(int)], nVars* sizeof(int));
	memcpy(gdim_meta, &buffer[2*nVars* sizeof(int)], MAX_DS_NDIM * nVars * sizeof(uint64_t));

	for (int var = 0; var < nVars; ++var) {

		int var_dim_size = dim_meta[var];
		//fprintf(stderr, "NDim: %d", var_dim_size);
		int varType = elemSize_meta[var];
		//fprintf(stderr, " Type: %d", varType);
		//fprintf(stderr, " Gdim: %llu\n", gdim_meta[0]);

		std::string adiosName;
		char *val = (char *)(calloc(var_name_max_length, sizeof(char)));
		memcpy(val, &buffer[nVars* sizeof(int) + nVars *sizeof(int) + nVars*MAX_DS_NDIM*sizeof(uint64_t) + var * var_name_max_length], var_name_max_length*sizeof(char));
		fprintf(stderr, "VarName: %s\n", val);
		adiosName.assign(val);
		free(val);

		Dims shape;
		shape.resize(var_dim_size);
		if (var_dim_size > 0)
		{
			bool isOrderC = helper::IsRowMajor(m_IO.m_HostLanguage);
			for (int i = 0; i < var_dim_size; i++)
			{
				if (isOrderC)
				{
					//uint64_t gdim_value = *(uint64_t*)buffer[]
					shape[var_dim_size - i - 1] = static_cast<int>(gdim_meta[var*MAX_DS_NDIM+i]);
				}
				else
				{
					shape[i] = static_cast<int>(gdim_meta[var*MAX_DS_NDIM+i]);;
				}
			}
		}
		std::string adiosVarType;
		auto itType = ds_to_varType.find(varType);
		if(itType == ds_to_varType.end()){
			fprintf(stderr, "Wrong value in the serialized meta buffer for varType\n");
			//TO DO fix for complex data type
		}else{
			adiosVarType = itType->second;

		}
		if(adiosVarType =="char")
			AddVar<char>(m_IO, adiosName, shape);
		else if(adiosVarType =="float")
				AddVar<float>(m_IO, adiosName, shape);
		else if(adiosVarType =="double")
				AddVar<double>(m_IO, adiosName, shape);
		else if(adiosVarType =="float complex")
				AddVar<std::complex<float>>(m_IO, adiosName, shape);
		else if(adiosVarType =="double complex")
				AddVar<std::complex<double>>(m_IO, adiosName, shape);
		else if(adiosVarType =="signed char")
				AddVar<signed char>(m_IO, adiosName, shape);
		else if(adiosVarType =="short")
				AddVar<short>(m_IO, adiosName, shape);
		else if(adiosVarType =="long int")
				AddVar<long int>(m_IO, adiosName, shape);
		else if(adiosVarType =="long long int")
				AddVar<long long int>(m_IO, adiosName, shape);
		else if(adiosVarType =="string")
				AddVar<std::string>(m_IO, adiosName, shape);
		else if(adiosVarType =="unsigned char")
				AddVar<unsigned char>(m_IO, adiosName, shape);
		else if(adiosVarType =="unsigned short")
				AddVar<unsigned short>(m_IO, adiosName, shape);
		else if(adiosVarType =="unsigned int")
				AddVar<unsigned int>(m_IO, adiosName, shape);
		else if(adiosVarType =="unsigned long int")
				AddVar<unsigned long int>(m_IO, adiosName, shape);
		else if(adiosVarType =="unsigned long long int")
				AddVar<unsigned long long int>(m_IO, adiosName, shape);
		else
			AddVar<int>(m_IO, adiosName, shape);// used int for last value

	}

	free(dim_meta);
	free(elemSize_meta);
	free(gdim_meta);
	free(buffer);
	delete[] local_str;
    return StepStatus::OK;
}


size_t DataSpacesReader::CurrentStep() const
{
    return m_CurrentStep;
}

void DataSpacesReader::EndStep()
{

	PerformGets();

	int rank;
	MPI_Comm_rank(m_data.mpi_comm, &rank);
	MPI_Barrier(m_data.mpi_comm);
    //Release lock in End Step
	char *cstr = new char[f_Name.length() + 1];
	strcpy(cstr, f_Name.c_str());
    MPI_Barrier(m_data.mpi_comm);
    dspaces_unlock_on_read(cstr, &m_data.mpi_comm);
    delete[] cstr;

}

void DataSpacesReader::DoClose(const int transportIndex){

	PerformGets();

	fprintf(stderr, "%s: Disconnect from DATASPACES server now, rank= ...\n", __func__);
	// disconnect from dataspaces if we are connected from writer but not anymore from reader
	if (globals_adios_is_dataspaces_connected_from_reader() &&
			!globals_adios_is_dataspaces_connected_from_writer())
	{
		MPI_Barrier (m_data.mpi_comm);
	}
	globals_adios_set_dataspaces_disconnected_from_reader();
}

void DataSpacesReader::Flush(const int transportIndex) {}

void DataSpacesReader::PerformGets()
{
	if(m_DeferredStack.size()>0){
	#define declare_type(T)                                                        \
	    for (std::string variableName : m_DeferredStack)                           \
	    {                                                                          \
	        Variable<T> *var = m_IO.InquireVariable<T>(variableName);              \
	        if (var != nullptr)                                                    \
	        {                                                                      \
	            ReadDsData(*var, var->GetData(), m_CurrentStep);                          \
	        }                                                                      \
	    }
	        ADIOS2_FOREACH_TYPE_1ARG(declare_type)
	#undef declare_type
	        return;
	    m_DeferredStack.clear();
	}
}


#define declare_type(T)                                                        \
    void DataSpacesReader::DoGetSync(Variable<T> &variable, T *data)                \
    {                                                                          \
        GetSyncCommon(variable, data);                                         \
    }                                                                          \
    void DataSpacesReader::DoGetDeferred(Variable<T> &variable, T *data)            \
    {                                                                          \
        GetDeferredCommon(variable, data);                                     \
    }

ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type



} // end namespace engine
} // end namespace core
} // end namespace adios2



