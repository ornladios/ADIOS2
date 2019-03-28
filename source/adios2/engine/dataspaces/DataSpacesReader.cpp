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
    auto appID = m_IO.m_Parameters.find("AppID");
	if (appID != m_IO.m_Parameters.end()){
		m_data.appid = std::stoi(appID->second);
	}else{
		m_data.appid = 0;

	}
    ret = adios_read_dataspaces_init(&mpiComm, &m_data);
    if(ret < 0){
    	fprintf(stderr, "DataSpaces did not initialize properly %d\n", ret);
    }

}

DataSpacesReader::~DataSpacesReader() { }


StepStatus DataSpacesReader::BeginStep(StepMode mode, const float timeout_sec)
{

	std::string ds_file_var;
	std::string local_file_var;
	uint64_t gdims[MAX_DS_NDIM], lb[MAX_DS_NDIM], ub[MAX_DS_NDIM];
	int elemsize, ndim, rank;
	//acquire lock, current step and n_vars in the Begin Step
	char *cstr;
	char *meta_lk;
	int bcast_array[2]={0,0};
	MPI_Comm_rank(m_data.mpi_comm, &rank);
	//MPI_Comm self_comm = MPI_COMM_SELF;

	char *fstr = new char[f_Name.length() + 1];
	strcpy(fstr, f_Name.c_str());

	char *buffer;
	int nVars=0;
	if(mode == StepMode::NextAvailable){
		dspaces_lock_on_read (fstr, &(m_data.mpi_comm));
		if(rank==0){
			buffer = dspaces_get_next_meta(m_CurrentStep, fstr, &bcast_array[0], &bcast_array[1]);
			//bcast_array[0]=nVars;
			//bcast_array[1]=m_CurrentStep;
		}
		dspaces_unlock_on_read (fstr, &(m_data.mpi_comm));
	}
	if(mode == StepMode::LatestAvailable){
		dspaces_lock_on_read (fstr, &(m_data.mpi_comm));
		if(rank==0){
				buffer = dspaces_get_latest_meta(m_CurrentStep, fstr, &bcast_array[0], &bcast_array[1]);
		}
		dspaces_unlock_on_read (fstr, &(m_data.mpi_comm));
	}
	MPI_Bcast(bcast_array, 2, MPI_INT, 0, m_data.mpi_comm);
	nVars = bcast_array[0];
	m_CurrentStep = bcast_array[1];
	//MPI_Bcast(&m_CurrentStep, 1, MPI_INT, 0, m_data.mpi_comm);
	//MPI_Bcast(&nVars, 1, MPI_INT, 0, m_data.mpi_comm);
	if(nVars==0)
			return StepStatus::EndOfStream;
	int var_name_max_length = 128;
	int buf_len = nVars * sizeof(int) + nVars * sizeof(int)+ MAX_DS_NDIM * nVars * sizeof(uint64_t) + nVars * var_name_max_length * sizeof(char);
	if(rank!=0)
			buffer = (char*)malloc(buf_len);

	m_IO.RemoveAllVariables();

	MPI_Bcast(buffer, buf_len, MPI_CHAR, 0, m_data.mpi_comm);
	//now populate data from the buffer

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
		int varType = elemSize_meta[var];

		std::string adiosName;
		char *val = (char *)(calloc(var_name_max_length, sizeof(char)));
		memcpy(val, &buffer[nVars* sizeof(int) + nVars *sizeof(int) + nVars*MAX_DS_NDIM*sizeof(uint64_t) + var * var_name_max_length], var_name_max_length*sizeof(char));
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

    return StepStatus::OK;
}


size_t DataSpacesReader::CurrentStep() const
{
    return m_CurrentStep;
}

void DataSpacesReader::EndStep()
{

	MPI_Barrier(m_data.mpi_comm);
	PerformGets();


}

void DataSpacesReader::DoClose(const int transportIndex){

	if (globals_adios_is_dataspaces_connected_from_reader() &&
				!globals_adios_is_dataspaces_connected_from_both())
	{
		//fprintf(stderr, "Disconnecting reader via finalize \n");
		MPI_Barrier (m_data.mpi_comm);
		dspaces_finalize();

	}
	globals_adios_set_dataspaces_disconnected_from_writer();

}

void DataSpacesReader::Flush(const int transportIndex) {}

void DataSpacesReader::PerformGets()
{
	if(m_DeferredStack.size()>0 && m_CurrentStep <= latestStep){
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



