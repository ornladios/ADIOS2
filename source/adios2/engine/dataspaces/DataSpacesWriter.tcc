/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataSpacesWriter.tcc
 *
 *  Created on: Dec 5, 2018
 *      Author: Pradeep Subedi
 *				pradeep.subedi@rutgers.edu
 */
#ifndef ADIOS2_ENGINE_DATASPACES_DATASPACESWRITER_TCC_
#define ADIOS2_ENGINE_DATASPACES_DATASPACESWRITER_TCC_

#include <memory>

#include "DataSpacesWriter.h"
#include "adios2/toolkit/dataspaces/ds_data.h"
#include "adios2/helper/adiosFunctions.h"
#include "dataspaces.h"

namespace adios2
{
namespace core
{
namespace engine
{


template <class T>
void DataSpacesWriter::DoPutSyncCommon(Variable<T> &variable, const T *values)
{


	 uint64_t lb_in[MAX_DS_NDIM], ub_in[MAX_DS_NDIM], gdims_in[MAX_DS_NDIM];
	//Lock is acquired in BeginStep() and will be released in EndStep()
	//For clients the lock is acquired in f_name type;
	//fprintf(stderr, "Starting to write data to dataspaces\n");
	 std::vector<uint64_t> dims_vec;

	unsigned int version;
	version = m_CurrentStep;
	int ndims = std::max(variable.m_Shape.size(), variable.m_Count.size());
	//fprintf(stderr, "ndims is %d\n", ndims);
	ndim_vector.push_back(ndims);
	bool isOrderC = helper::IsRowMajor(m_IO.m_HostLanguage);
	/* Order of dimensions: in DataSpaces: fast --> slow --> slowest
	       For example:
	       Fortran: i,j,k --> i, j, k  = lb[0], lb[1], lb[2]
	                i,j   --> i, j     = lb[0], lb[1]
	                i     --> i        = lb[0]
	       C:       i,j,k --> k, j, i  = lb[2], lb[1], lb[0]
	                i,j   --> j, i     = lb[1], lb[0]
	                i     --> i        = lb[0]
	    */

	    if (isOrderC)
	    {
	            for (int i = 0; i < ndims; i++)
				{
					gdims_in[i] = static_cast<uint64_t>(variable.m_Shape[ndims - i - 1]);
					//fprintf(stderr, "%d", gdims_in[i]);
					dims_vec.push_back(gdims_in[i]);
					lb_in[i] = static_cast<uint64_t>(variable.m_Start[ndims - i - 1]);
					ub_in[i] = static_cast<uint64_t>(variable.m_Start[ndims - i - 1]+variable.m_Count[ndims - i - 1]-1);
				}

	    }
	    else{

	    	for (int i = 0; i < ndims; i++)
			{
				gdims_in[i] = static_cast<uint64_t>(variable.m_Shape[i]);
				//fprintf(stderr, "%d", gdims_in[i]);
				dims_vec.push_back(gdims_in[i]);
				lb_in[i] = static_cast<uint64_t>(variable.m_Start[i]);
				ub_in[i] = static_cast<uint64_t>(variable.m_Start[i]+variable.m_Count[i]-1);
			}
	    }
	    gdims_vector.push_back(dims_vec);
	    int varType;
	    auto itType = varType_to_ds.find(variable.m_Type);
	    if(itType == varType_to_ds.end()){
	    	varType = 2;
	    	fprintf(stderr, "variable Type not found. Using Integer as data type");
	    	//Might have to fix for complex data types
	    }else{
	    	varType = itType->second;

	    }
	    elemSize_vector.push_back(varType);
	    std::string ds_in_name = f_Name;
	    ds_in_name +=  variable.m_Name;
	    v_name_vector.push_back(variable.m_Name);
	    char *var_str = new char[ds_in_name.length() + 1];
	    strcpy(var_str, ds_in_name.c_str());
	    variable.SetData(values);
	    //fprintf(stderr, "Writer Varname: %s, Gdim: %llu, lb: %llu, ub: %llu, ElementType:%d\n", var_str, gdims_in[0], lb_in[0], ub_in[0], elemSize_vector[0]);
	    dspaces_define_gdim(var_str, ndims, gdims_in);
	    dspaces_put(var_str, version, variable.m_ElementSize, ndims, lb_in, ub_in, values);
	    dspaces_put_sync();
	    delete[] var_str;

	    //fprintf(stderr, "Finished writing data to dataspaces\n");

}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif 

