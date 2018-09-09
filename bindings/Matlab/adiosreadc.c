/* 
 * ADIOS is freely available under the terms of the BSD license described
 * in the COPYING file in the top level directory of this source distribution.
 *
 * Copyright (c) 2008 - 2009.  UT-BATTELLE, LLC. All rights reserved.
 */

/*=================================================================
 * adiosreadc.c - read a variable/attribute from an ADIOS file
 *
 * Input: File, Group, Path, Offset, Count, Verbose
 *    File:     string (name) or int64 (handler)
 *    Group:    string (name) or int64 (handler) or int32 (index)
 *    Path:     string (variable/attribute name with full path)
 *    Offset:   int32 array 
 *    Count:    int32 array
 *    Verbose:  numeric (double)
 * Output: The variable/attribute as mxArray
 *
 *
 * $Revision: 1.0 $  $Date: 2009/08/05 12:53:41 $
 * Author: Norbert Podhorszki <pnorbert@ornl.gov>
 *=================================================================*/

#include <string.h>    /* strlen */
#include "mex.h"
#include "adios_read.h"
#include "adios_types.h"

static int verbose=0;

mxArray* readdata( ADIOS_GROUP *gp, const char *path, int in_noffsets, 
                   const int64_t *in_offsets, const int64_t *in_counts);
void errorCheck(int nlhs, int nrhs, const mxArray *prhs[]);
char* getString(const mxArray *mxstr);
mxClassID adiostypeToMatlabClass(enum ADIOS_DATATYPES type, mxComplexity *complexity);
mxArray* createMatlabArray( enum ADIOS_DATATYPES adiostype, int ndims, uint64_t *dims); 
void recalc_offsets( int ndims, uint64_t *dims, mwSize in_noffsets, 
                     const int64_t *in_offsets, const int64_t *in_counts,
                     uint64_t *offsets, uint64_t *counts);
static void swap_order(int n, uint64_t *array);



void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
    char *fname;        /* file name */
    char *gname;        /* group name */
    char *path;         /* name of variable or attribute */
    mwSize in_noffsets;    /* number of offsets provided, = number of counts */
    int64_t *in_offsets;    /* offset array provided */
    int64_t *in_counts;     /* count array provided */
    int status;
    char msg[512];        /* error messages from function calls */

    int64_t fh, gh, *int64p; /* file and group handlers and temp pointers */
    int32_t *int32p;
    int  haveToCloseFile, haveToCloseGroup, readAttributesToo;

    ADIOS_FILE *fp;
    ADIOS_GROUP *gp;
    int mpi_comm_dummy; 

    int i;
    mxArray *out;        /* output data array */
    const char *field_names[] = {"name", "value"}; /* 2nd output arg's struct fields */
    mwSize attr_struct_dims[2];   /* dimensions for 2nd arg struct array: 1-by-sth */

    errorCheck(nlhs, nrhs, prhs);

    /*mexPrintf("nrhs=%d  nlhs=%d\n", nrhs, nlhs);*/
    
    /***********************/
    /* 1. get verbose parameter first */
    verbose = (int)mxGetScalar(prhs[5]);
    if (verbose) mexPrintf("Verbosity level: %d\n", verbose);

    /***********************/
    /* 1. get file handler */
    fp = NULL;
    haveToCloseFile = 0;
    if (mxIsChar(prhs[0])) {
        fname = getString( (mxArray *)prhs[0]);
        if (strlen(fname) > 0) {
            if (verbose) mexPrintf("File name: \"%s\"\n", fname);
            /* Open ADIOS file now */
            fp = adios_fopen (fname, mpi_comm_dummy);
            if (fp == NULL) {
                mexErrMsgIdAndTxt("MATLAB:adiosreadc:open",adios_errmsg());
            }
            haveToCloseFile = 1;
        }
    } 


    /************************/
    /* 2. get group handler */
    if (mxIsInt64(prhs[1])) { /* int64 group handler provided */
        int64p = (int64_t *) mxGetData(prhs[1]); 
        gp = (ADIOS_GROUP *) *int64p;
        mexPrintf("Group gp=%lld id=%d vcnt=%d\n", (int64_t)gp, gp->grpid, gp->vars_count);
        haveToCloseGroup = 0;
    } else {
        /* reference to a group in file */
        if (fp == NULL) {
            mexErrMsgIdAndTxt("MATLAB:adiosreadc:open",
               "Group argument must be an open group struct if File argument is not provided\n");
        }

        if (mxIsChar(prhs[1])) {
            gname = getString( (mxArray *)prhs[1]);
        if (verbose) mexPrintf("Group name: \"%s\"\n", gname );

            /* empty groupname -> first group in the file */
            if ( strlen(gname)==0 || !strcmp(gname,"/") || !strcmp(gname," ")) {
                gp = adios_gopen_byid(fp, 0);
                if (gp == NULL) {
                    mexErrMsgIdAndTxt("MATLAB:adiosreadc:open",adios_errmsg());
                }
            } else {
                /* Open group by name */
                gp = adios_gopen(fp, gname);
                if (gp == NULL) {
                    mexErrMsgIdAndTxt("MATLAB:adiosreadc:open",adios_errmsg());
                }
            }
            haveToCloseGroup = 1;

        } else if (mxIsInt32(prhs[1])) { /* group index provided */
            int32p  = (int32_t *) mxGetData(prhs[1]); 
            if (verbose) mexPrintf("Group index: %d\n", *int32p);
            gname = NULL;

            /* group index in Matlab starts from 1, in adios starts from 0 */
            /* Open group by index */
            gp = adios_gopen_byid(fp, *int32p-1);
            if (gp == NULL) {
                mexErrMsgIdAndTxt("MATLAB:adiosreadc:open",adios_errmsg());
            }
            haveToCloseGroup = 1;
        }
    }

    /*****************************************************************************************/
    /* 3. get other arguments: char variable name, int64 in_offsets[], int64 in_counts[] */
    path     = getString(prhs[2]);
    if (verbose) mexPrintf("Variable name: \"%s\"\n", path );
    in_noffsets = mxGetNumberOfElements(prhs[3]);
    in_offsets  = (int64_t *) mxGetData(prhs[3]); 
    in_counts   = (int64_t *) mxGetData(prhs[4]); 


    /*********************************************************************/
    /* 4. read in variable */
    out = readdata( gp, path, in_noffsets, in_offsets, in_counts);
    if ( nlhs >= 1 ) {
        plhs[0] = out;
    }

    /********************************************************/
    /* 5. read in all attributes of a variable if requested */
#if 0
    if ( nlhs == 2 ) {
        if (!isvar) {
            mexErrMsgIdAndTxt("MATLAB:adiosreadc:read",
               "Second output argument can be provided only for variables. Path %s refers to an attribute.",
               path);
        } else {
            /* Read all attributes directly under the given path (= path/name) */
            if (verbose) mexPrintf("Read in attributes under the path %s\n", path);
            nattrs = adios_readutil_getdirectattributes(gh, path, &attrlist); 
            if (verbose) mexPrintf("Found %d matching attributes\n", nattrs);
            attr_struct_dims[0] = 1;
            attr_struct_dims[1] = nattrs;
            /* Create a 1-by-n array of structs with fields {name,value}. */ 
            plhs[1] = mxCreateStructArray(2, attr_struct_dims, 2, field_names);
            if (verbose) mexPrintf("Created struct array, 1-by-%d\n", attr_struct_dims[1]);
            i=0;
            attr=attrlist;
            while (attr != NULL) {
                mxSetFieldByNumber(plhs[1],i,0,mxCreateString(attr->name));
                out = readdata( fh, gh, attr->name, 0, NULL, NULL, timestep, &isvar);
                mxSetFieldByNumber(plhs[1],i,1,out);
                if (verbose) mexPrintf("Added attr: %s\n", attr->name);
                i++;
                attr=attr->next;
            }
            adios_readutil_freenamelist(attrlist);
        }
    }
#endif


    /**************************************************/
    /* 6. close group and file if opened in this call */
    if (haveToCloseGroup) {
        if (verbose) mexPrintf("Close group\n");
        adios_gclose(gp);
        if (gname) 
            mxFree(gname);
    }

    if (haveToCloseFile) {
        if (verbose) mexPrintf("Close file\n");
        adios_fclose(fp);
        if (fname)
            mxFree(fname);
    }


    mxFree(path);

}

mxArray* readdata( ADIOS_GROUP *gp, const char *path, mwSize in_noffsets,
                   const int64_t *in_offsets, const int64_t *in_counts) 
{
    /* FIXME: does not work for COMPLEX data because
       adios stores real and imaginary together as Fortran Complex type does, 
       while matlab stores them as two separate float/double arrays
    */
    void *data;             /* content of variable read in */
    uint64_t offsets[16], counts[16]; /* extended offsets/counts */
    int64_t read_bytes;
    uint64_t arraysize;
    int  status, i;
    mxArray * out;
    char msg[512];
    ADIOS_VARINFO *vinfo;

    /* get type/size info on variable */
    if (verbose) mexPrintf("Get info on var: %s\n", path);
    vinfo = adios_inq_var( gp, path);
    if (vinfo == NULL) {
        mexErrMsgIdAndTxt("MATLAB:adiosreadc:varinfo",adios_errmsg());
    }
    if (verbose) { 
        mexPrintf("Var=%s type=%s C dimensions %dD [", path, adios_type_to_string(vinfo->type), vinfo->ndim);
        for (i=0; i<vinfo->ndim; i++)
            mexPrintf("%lld ", vinfo->dims[i]);
        mexPrintf("]\n");
    }

    /* Flip dimensions from ADIOS-read-api/C/row-major order to Matlab/Fortran/column-major order */
    swap_order(vinfo->ndim, vinfo->dims);

    /* extend offsets/counts if needed, change from 1..N to 0..N-1 indexing and
       interpret negative indices */
    recalc_offsets( vinfo->ndim, vinfo->dims, in_noffsets, in_offsets, in_counts, offsets, counts);

    /* create Matlab array with the appropriate type and size */
    if (vinfo->type == adios_string) {
        /* Matlab string != char array of C, so handle separately */
        data = (void *) mxCalloc(vinfo->dims[0], sizeof(char));
    } else {
        if (verbose) { 
            mexPrintf("Create %d-D Matlab array [", vinfo->ndim);
            for (i=0; i<vinfo->ndim; i++)
                mexPrintf("%lld ", counts[i]);
            mexPrintf("]\n");
        }
        out = createMatlabArray(vinfo->type, vinfo->ndim, counts); 
        data = (void *) mxGetData(out);
    }

    /* Flip offsets/counts from Matlab/Fortran/column-major order to ADIOS-read-api/C/row-major order */
    swap_order(vinfo->ndim, offsets);
    swap_order(vinfo->ndim, counts);

    /* read in data */
    if (verbose) mexPrintf("Read in data\n");
    
    read_bytes = adios_read_var_byid( gp, vinfo->varid, offsets, counts, data);
    if (read_bytes < 0) {
        mexErrMsgIdAndTxt("MATLAB:adiosreadc:read",adios_errmsg());
    }
    if (verbose) mexPrintf("Read in %lld bytes\n", read_bytes);

    if (vinfo->type == adios_string) {
        out = mxCreateString( (char *)data);
        mxFree(data);
    }

    adios_free_varinfo(vinfo);
    return out;
}
    

void errorCheck(int nlhs, int nrhs, const mxArray *prhs[]){
    /* Assume that we are called from adiosread.m which checks the arguments already */
    /* Check for proper number of arguments. */
    
    if ( nrhs != 6 ) {
        mexErrMsgIdAndTxt("MATLAB:adiosreadc:rhs","This function needs exactly 6 arguments: File, Group, Varpath, Offsets, Counts, Verbose");
    }
    
    if ( !mxIsChar(prhs[0]) && !mxIsInt64(prhs[0]) ) {
        mexErrMsgIdAndTxt("MATLAB:adiosreadc:rhs","First arg must be either a string or an int64 handler.");
    } 
    
    if ( nlhs > 1 ) {
        mexErrMsgIdAndTxt("MATLAB:adiosreadc:lhs","Too many output arguments.");
    }
    
#if !defined(MX_COMPAT_32)
    /* Make sure that it is safe to cast dim to mwSize when using largeArrayDims.*/
    if ( dim > MWSIZE_MAX ) {
        mexErrMsgIdAndTxt("MATLAB:adiosreadc:dimensionTooLarge",
                          "The input dimension, %.0f, is larger than the maximum value of mwSize, %u, when built with largeArrayDims.", dim, MWSIZE_MAX);
    }
#endif
 }


/** Make a C char* string from a Matlab string */
char* getString(const mxArray *mxstr) 
{
    mwSize buflen;
    char   *str;
    /* Allocate enough memory to hold the converted string. */
    buflen = mxGetNumberOfElements(mxstr) + 1;
    str = mxCalloc(buflen, sizeof(char));
    /* Copy the string data from string_array_ptr and place it into buf. */
    if (mxGetString(mxstr, str, buflen) != 0)
        mexErrMsgTxt("Could not convert string data from the file name.");
    return str;
}

/** return the appropriate class for an adios type */
mxClassID adiostypeToMatlabClass(enum ADIOS_DATATYPES type, mxComplexity *complexity) 
{
    *complexity=mxREAL;
    switch( type ) {
        case adios_unsigned_byte:
            return mxUINT8_CLASS;
        case adios_byte:
            return mxINT8_CLASS;
        case adios_string:
            return mxCHAR_CLASS;
               
        case adios_unsigned_short:
            return mxUINT16_CLASS;
        case adios_short:
            return mxINT16_CLASS;

        case adios_unsigned_integer:
            return mxUINT32_CLASS;
        case adios_integer:
            return mxINT32_CLASS;

        case adios_unsigned_long:
            return mxUINT64_CLASS;
        case adios_long:
            return mxINT64_CLASS;

        case adios_real: 
            return mxSINGLE_CLASS;
        case adios_double:
            return mxDOUBLE_CLASS;
             
        case adios_complex:     /* 8 bytes */
            *complexity = mxCOMPLEX;
            return mxSINGLE_CLASS;
        case adios_double_complex: /*  16 bytes */
            *complexity = mxCOMPLEX;
            return mxDOUBLE_CLASS;

        case adios_long_double: /* 16 bytes */
        default:
            mexErrMsgIdAndTxt("MATLAB:adiosreadc.c:dimensionTooLarge",
                 "Adios type %d (%s) not supported in visit.\n",
                 type, adios_type_to_string(type));
            break;
    }
    return 0; /* just to avoid warnings. never executed */
}

/* create an N-dim array with given type and dimensions */
mxArray* createMatlabArray( enum ADIOS_DATATYPES adiostype, int ndims, uint64_t *dims)
{
    mxClassID mxtype;
    mxArray *arr;
    mwSize  mNdims;
    mwSize  mDims[16];
    mxComplexity ComplexFlag;
    int     i;
    int     isTypeComplex;

    /* convert ints to mwSizes */
    if (ndims > 0) {
        mNdims = (mwSize) ndims;
        for (i=0; i<ndims; i++) 
            mDims[i] = (mwSize)dims[i];
    } else {
        /* 0 dim: scalar value -> 1-by-1 Matlab array */
        mNdims = 2;
        mDims[0] = 1;
        mDims[1] = 1;
    }

    /* get type */
    mxtype = adiostypeToMatlabClass(adiostype, &ComplexFlag);

    /* create array */
    arr = mxCreateNumericArray( mNdims, mDims, mxtype, ComplexFlag);

    if (verbose) mexPrintf("Array for adios type %s is created\n", adios_type_to_string(adiostype));

    return arr;
}


/** - extend offset/count arrays to ndims if needed 
    - recalculate "from 1" Matlab indices to "from 0" C indices
    - recalculate the negative indices 
    !!! Provide the output arrays in the caller !!!
*/
void recalc_offsets( int ndims, uint64_t *dims, mwSize in_noffsets, 
                     const int64_t *in_offsets, const int64_t *in_counts,
                     uint64_t *offsets, uint64_t *counts)
{
    int i;
    for (i=0; i<ndims; i++) {
        if ((mwSize)i < in_noffsets) {
            if (in_offsets[i] < 0) /* negative offset means last-|offset| */
                offsets[i] = dims[i] + in_offsets[i];
            else 
                offsets[i] = in_offsets[i] - 1; /* C index start from 0 */

            if (in_counts[i] < 0) /* negative count means last-|count|+1-start */
                counts[i]  = dims[i] + in_counts[i] - offsets[i] + 1;
            else
                counts[i]  = in_counts[i]; 
        } else {
            /* extend offset/count array to match variable's dimensions */
            if (verbose) mexPrintf("Extend offset/counts for dim %d: offset=%d count=%d\n", i, 0, dims[i] );
            offsets[i] = 0;
            counts[i]  = dims[i];
        }

    }
}

/* Reverse the order in an array in place.
   use swapping from Matlab/Fortran/column-major order to ADIOS-read-api/C/row-major order and back
*/
static void swap_order(int n, uint64_t *array)
{
    int i, tmp;
    for (i=0; i<n/2; i++) {
        tmp = array[i];
        array[i] = array[n-1-i];
        array[n-1-i] = tmp;
    }
}




