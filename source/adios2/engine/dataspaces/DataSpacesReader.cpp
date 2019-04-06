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
	//acquire lock, current step and n_vars in the Begin Step

	std::string ds_file_var, local_file_var;
	int elemsize, ndim, rank;
	int bcast_array[2]={0,0};
	uint64_t gdims[MAX_DS_NDIM], lb[MAX_DS_NDIM], ub[MAX_DS_NDIM];

	char *cstr, *meta_lk, *buffer;

	MPI_Comm_rank(m_data.mpi_comm, &rank);
	char *fstr = new char[f_Name.length() + 1];
	strcpy(fstr, f_Name.c_str());

	std::string lk_name = f_Name + std::to_string(m_CurrentStep+1);
	meta_lk = new char[lk_name.length() + 1];
	strcpy(meta_lk, lk_name.c_str());

	int nVars=0;
	if(mode == StepMode::NextAvailable){
		dspaces_lock_on_read (meta_lk, &(m_data.mpi_comm));
		if(rank==0){
			buffer = dspaces_get_next_meta(m_CurrentStep, fstr, &bcast_array[0], &bcast_array[1]);
		}
		dspaces_unlock_on_read (meta_lk, &(m_data.mpi_comm));
	}
	if(mode == StepMode::LatestAvailable){
		dspaces_lock_on_read (meta_lk, &(m_data.mpi_comm));
		if(rank==0){
				buffer = dspaces_get_latest_meta(m_CurrentStep, fstr, &bcast_array[0], &bcast_array[1]);
		}
		dspaces_unlock_on_read (meta_lk, &(m_data.mpi_comm));
	}
	MPI_Bcast(bcast_array, 2, MPI_INT, 0, m_data.mpi_comm);
	nVars = bcast_array[0];
	m_CurrentStep = bcast_array[1];
	if(nVars==0)
			return StepStatus::EndOfStream;
	int var_name_max_length = 128;
	int buf_len = sizeof(int) + nVars * sizeof(int) + nVars * sizeof(int)+ MAX_DS_NDIM * nVars * sizeof(uint64_t) + nVars * var_name_max_length * sizeof(char);
	if(rank!=0)
			buffer = (char*)malloc(buf_len);

	m_IO.RemoveAllVariables();

	MPI_Bcast(buffer, buf_len, MPI_CHAR, 0, m_data.mpi_comm);
	//now populate data from the buffer

	int *dim_meta, *elemSize_meta;
	uint64_t *gdim_meta;
	dim_meta = (int*) malloc(nVars* sizeof(int));
	elemSize_meta = (int*) malloc(nVars*sizeof(int));
	gdim_meta = (uint64_t *)malloc(MAX_DS_NDIM * nVars * sizeof(uint64_t));
	memset(gdim_meta, 0, MAX_DS_NDIM * nVars * sizeof(uint64_t));

	memcpy(dim_meta, &buffer[sizeof(int)],  nVars* sizeof(int));
	memcpy(elemSize_meta, &buffer[sizeof(int)+ nVars* sizeof(int)], nVars* sizeof(int));
	memcpy(gdim_meta, &buffer[sizeof(int) + 2*nVars* sizeof(int)], MAX_DS_NDIM * nVars * sizeof(uint64_t));

	for (int var = 0; var < nVars; ++var) {

		int var_dim_size = dim_meta[var];
		int varType = elemSize_meta[var];

		std::string adiosName;
		char *val = (char *)(calloc(var_name_max_length, sizeof(char)));
		memcpy(val, &buffer[sizeof(int) + nVars* sizeof(int) + nVars *sizeof(int) + nVars*MAX_DS_NDIM*sizeof(uint64_t) + var * var_name_max_length], var_name_max_length*sizeof(char));
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
		if(adiosVarType =="int8_t")
			AddVar<int8_t>(m_IO, adiosName, shape);
		else if(adiosVarType =="uint8_t")
			AddVar<uint8_t>(m_IO, adiosName, shape);
		else if(adiosVarType =="int16_t")
			AddVar<int16_t>(m_IO, adiosName, shape);
		else if(adiosVarType =="uint16_t")
						AddVar<uint16_t>(m_IO, adiosName, shape);
		else if(adiosVarType =="int32_t")
						AddVar<int32_t>(m_IO, adiosName, shape);
		else if(adiosVarType =="uint32_t")
						AddVar<uint32_t>(m_IO, adiosName, shape);
		else if(adiosVarType =="int64_t")
						AddVar<int64_t>(m_IO, adiosName, shape);
		else if(adiosVarType =="uint64_t")
						AddVar<uint64_t>(m_IO, adiosName, shape);
		else if(adiosVarType =="float")
				AddVar<float>(m_IO, adiosName, shape);
		else if(adiosVarType =="double")
				AddVar<double>(m_IO, adiosName, shape);
		else if(adiosVarType =="long double")
				AddVar<long double>(m_IO, adiosName, shape);
		else if(adiosVarType =="float complex")
				AddVar<std::complex<float>>(m_IO, adiosName, shape);
		else if(adiosVarType =="double complex")
				AddVar<std::complex<double>>(m_IO, adiosName, shape);
		else
			AddVar<std::string>(m_IO, adiosName, shape);// used string for last value

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
	        ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
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

ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type



} // end namespace engine
} // end namespace core
} // end namespace adios2



