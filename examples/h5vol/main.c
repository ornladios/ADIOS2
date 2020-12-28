#include "H5Vol.h"

#include "stdlib.h"

#define VARNAME1 "Var1"
#define VARNAME2 "Var2"

#define ATTRNAME_INT "IntAttr"
#define ATTRNAME_CHAR "CharAttr"
#define ATTRNAME_STRING "StringAttr"
#define ATTRNAME_FLOAT_1D "Float1DAttr"
#define ATTRNAME_STRING_1D "String1DAttr"

#define GROUPNAME "Group"
#define SUBGROUPNAME "subGroup"

#define NX 32 /* dataset dimensions */
#define NY 32
#define DIM 2
#define LAZYRANK 3
#define NUM_STEPS 3
#define MAX_STRING_SIZE 30

//#define NATIVE_HDF5
void initIntData(int mpi_rank, int step, int **dataPtr, hsize_t dataSize)
{
    int i = 0;
    // Initialize data buffer
    if (mpi_rank == LAZYRANK)
    {
        *dataPtr = NULL;
    }
    else
    {
        *dataPtr = (int *)malloc(sizeof(int) * dataSize);
        for (i = 0; i < dataSize; i++)
        {
            (*dataPtr)[i] = mpi_rank + 10 + 100 * step;
        }
    }
}
hid_t defineArrayVar(int ndim, const hsize_t *dims, hid_t owner_id,
                     const char *varName, hid_t varType)
{
    hid_t filespace1 = H5Screate_simple(ndim, dims, NULL);

    hid_t dset_id = H5Dcreate(owner_id, varName, varType, filespace1,
                              H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    H5Sclose(filespace1);

    return dset_id;
}

void readScalarAttr(hid_t owner_id, const char *attrName, void *buf)
{
    hid_t attr = H5Aopen(owner_id, attrName, H5P_DEFAULT);
    if (-1 != attr)
    {
        hid_t attrType = H5Aget_type(attr);
        hid_t ret = H5Aread(attr, attrType, buf);
        ret = H5Aclose(attr);
    }
}

size_t readArrayAttr(hid_t owner_id, const char *attrName, void **buf)
{
    hid_t attr = H5Aopen(owner_id, attrName, H5P_DEFAULT);
    hid_t ret;
    if (-1 != attr)
    {
        hid_t attrType = H5Aget_type(attr);
        hid_t space = H5Aget_space(attr);
        hsize_t rank = H5Sget_simple_extent_ndims(space);
        size_t npoints = H5Sget_simple_extent_npoints(space);
        size_t typeSize = H5Tget_size(attrType);
        if (H5Tget_class(attrType) == H5T_STRING)
        {
            typeSize = MAX_STRING_SIZE;
            int k;
            /*
             *buf = malloc(sizeof(char*) * (int)npoints);
             for (k=0; k<npoints; k++)
               ((char**)(*buf))[k] = malloc(typeSize);
            */
            // char out[typeSize * npoints+1];
            char *out = (char *)(malloc(typeSize * npoints + 1));
            ret = H5Aread(attr, attrType, &out);
        }
        else
        {
            *buf = malloc(typeSize * (int)npoints);
            ret = H5Aread(attr, attrType, *buf);
        }
        ret = H5Aclose(attr);
        H5Sclose(space);
        return npoints;
    }
    return 0;
}
void writeScalarAttr(hid_t owner_id, hid_t attrType, const char *attrName,
                     const void *buf)
{
    hid_t a2_sid = H5Screate(H5S_SCALAR);
    hid_t attr2 = H5Acreate(owner_id, attrName, attrType, a2_sid, H5P_DEFAULT,
                            H5P_DEFAULT);

    H5Awrite(attr2, attrType, buf);

    H5Sclose(a2_sid);
    H5Aclose(attr2);
}

void writeArrayAttr(hid_t owner_id, int ndim, hid_t attrType,
                    const hsize_t *dimSpec, const char *attrName,
                    const void *buf)
{
    hid_t sid = H5Screate(H5S_SIMPLE);
    hid_t ret = H5Sset_extent_simple(sid, ndim, dimSpec, NULL);
    hid_t attr1 =
        H5Acreate(owner_id, attrName, attrType, sid, H5P_DEFAULT, H5P_DEFAULT);
    H5Awrite(attr1, attrType, buf);
    H5Sclose(sid);
    H5Aclose(attr1);
}

void AttrReadTests(hid_t owner_id)
{
    // read and display string scalar attr
    // true (>0) false(=0) fail(<0)
    htri_t result = -1;
    // H5Adelete(owner_id, ATTRNAME_STRING);
    result = H5Aexists(owner_id, ATTRNAME_STRING);

    if (result > 0)
    {
        char strValue[MAX_STRING_SIZE];
        readScalarAttr(owner_id, ATTRNAME_STRING, &strValue);
        printf("\t=> Read Attr %s, value %s\n", ATTRNAME_STRING, strValue);
    }

    size_t i;
    // read and display string array attr
    result = H5Aexists(owner_id, ATTRNAME_STRING_1D);
#ifdef NEVER
    if (result > 0)
    {
        char **strArray2;
        size_t n = readArrayAttr(owner_id, /*strType,*/ ATTRNAME_STRING_1D,
                                 (void **)(&strArray2));
        // size_t n = readArrayAttr(owner_id, /*strType,*/ ATTRNAME_STRING_1D,
        // (void**)(strArray2));
        for (i = 0; i < n; i++)
            printf("\t=> %s[%lu]=%s %lu\n", ATTRNAME_STRING_1D, i, strArray2[i],
                   strlen(strArray2[i]));

        for (i = 0; i < n; i++)
            free(strArray2[i]);
        if (n > 0)
            free(strArray2);
    }
#else
    if (result > 0)
    {
        // size_t n = readArrayAttr(owner_id, /*strType,*/ ATTRNAME_STRING_1D,
        // NULL);
    }
#endif

    // read and display int attr
    result = H5Aexists(owner_id, ATTRNAME_INT);
    if (result > 0)
    {
        int iVal;
        readScalarAttr(owner_id, /*H5T_NATIVE_INT,*/ ATTRNAME_INT, &iVal);
        printf("\t=> Read Attr %s, value %d\n", ATTRNAME_INT, iVal);
    }

    result = H5Aexists(owner_id, ATTRNAME_CHAR);
    if (result > 0)
    {
        char cVal;
        readScalarAttr(owner_id, /*H5T_NATIVE_CHAR,*/ ATTRNAME_CHAR, &cVal);
        printf("\t=> Read Attr %s, value %d\n", ATTRNAME_CHAR, cVal);
    }

    result = H5Aexists(owner_id, ATTRNAME_FLOAT_1D);
    if (result > 0)
    {
        float *fVal;
        size_t arraySize =
            readArrayAttr(owner_id, /*H5T_NATIVE_FLOAT,*/ ATTRNAME_FLOAT_1D,
                          (void **)(&fVal));

        for (i = 0; i < arraySize; i++)
            printf("\t=> %s[%lu] = [%.3f] \n", ATTRNAME_FLOAT_1D, i, fVal[i]);

        if (arraySize > 0)
            free(fVal);
    }
}

void AttrWriteTests(hid_t owner_id)
{
    { // test attributes
        printf("\n\t=> Now write a 1D data array attr \n");
        hsize_t a1_dim[] = {5};
        float a1Value[5] = {0.1, 0.2, 0.3, 0.04, -0.005};
        writeArrayAttr(owner_id, 1, H5T_NATIVE_FLOAT, a1_dim, ATTRNAME_FLOAT_1D,
                       a1Value);

        printf("\n\t=> Now write a scalar attr \n");
        int iValue = 9;
        writeScalarAttr(owner_id, H5T_NATIVE_INT, ATTRNAME_INT, &iValue);

        printf("\nAttempt to create a 2D attr,  and expect to FAIL\n");
        hsize_t a3_dim[] = {2, 2};
        float a3Value[2][2] = {0.1, 0.2, 0.3, 0.05};
        writeArrayAttr(owner_id, 2, H5T_NATIVE_FLOAT, a3_dim, "TopAttr2DFloat",
                       a3Value);

        printf("\n\t=> Now write a string attr \n");
        hid_t a4_sid = H5Screate(H5S_SCALAR);
        hid_t atype = H5Tcopy(H5T_C_S1);
        H5Tset_size(atype, 7);
        H5Tset_strpad(atype, H5T_STR_NULLTERM);
        hid_t attr4 = H5Acreate(owner_id, ATTRNAME_STRING, atype, a4_sid,
                                H5P_DEFAULT, H5P_DEFAULT);
        H5Awrite(attr4, atype, "string");
        H5Sclose(a4_sid);
        H5Aclose(attr4);
        H5Tclose(atype);

        printf("\n\t=> Now write a char attr \n");
        writeScalarAttr(owner_id, H5T_NATIVE_CHAR, ATTRNAME_CHAR, "a");

        printf("\n\t=> Now write a string array attr \n");
        hid_t a6_sid = H5Screate(H5S_SIMPLE);
        hsize_t a6_dim[] = {4};
        hid_t ret = H5Sset_extent_simple(a6_sid, 1, a6_dim, NULL);
        hid_t str = H5Tcopy(H5T_C_S1);
        H5Tset_size(str, H5T_VARIABLE);
        H5Tset_strpad(str, H5T_STR_NULLTERM);
        char *strArray[4] = {"Never", "let", "passion go", "away"};

        hid_t attr6 = H5Acreate(owner_id, ATTRNAME_STRING_1D, str, a6_sid,
                                H5P_DEFAULT, H5P_DEFAULT);

        H5Awrite(attr6, str, strArray);
        H5Sclose(a6_sid);
        H5Aclose(attr6);
        H5Tclose(str);
    }
}

hid_t GetFileID(MPI_Comm comm, MPI_Info info, const char *filename,
                bool doWrite)
{
    hid_t plist_id = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_fapl_mpio(plist_id, comm, info);
#ifdef NATIVE_HDF5
#else
    H5VL_ADIOS2_set(plist_id);
#endif
    hid_t file_id;
    if (doWrite)
        file_id = H5Fcreate(filename, H5F_ACC_TRUNC, H5P_DEFAULT, plist_id);
    else
        file_id = H5Fopen(filename, H5F_ACC_RDWR, plist_id);

    H5Pclose(plist_id);

    return file_id;
}

int TestW_1(const char *filename, MPI_Comm comm, MPI_Info info, int mpi_size,
            int mpi_rank)
{
    if (mpi_rank == 0)
        printf("===> Testing timestep write with LazyRank = %d <=== %s\n",
               LAZYRANK, filename);

    hid_t file_id, dset_id;
    hid_t filespace, memspace;
    hsize_t dimsf[2];
    int *data;

    hsize_t count[2];
    hsize_t offset[2];

    hid_t plist_id;
    int i;
    herr_t status;

    file_id = GetFileID(comm, info, filename, true);
    if (file_id < 0)
        return -1;

    if (mpi_rank == 0)
        AttrWriteTests(file_id);

    //
    // define dataset, will be used in the timesteps next
    //
    dimsf[0] = NX;
    dimsf[1] = NY;

    dset_id = defineArrayVar(DIM, dimsf, file_id, VARNAME1, H5T_NATIVE_INT);

    // devide data evenly in each process in memory and writes it to the
    // hyperslab skip writting when  rank == LAZYRANK
    count[0] = dimsf[0] / mpi_size;
    count[1] = dimsf[1];
    offset[0] = mpi_rank * count[0];
    offset[1] = 0;

    memspace = H5Screate_simple(DIM, count, NULL);
    if (mpi_rank == LAZYRANK)
        H5Sselect_none(memspace);

    // Select hyperslab in the file.
    filespace = H5Dget_space(dset_id);
    H5Sselect_hyperslab(filespace, H5S_SELECT_SET, offset, NULL, count, NULL);

    if (mpi_rank == LAZYRANK)
        H5Sselect_none(filespace);

    // Create property list for collective dataset write.
    plist_id = H5Pcreate(H5P_DATASET_XFER);
    H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_COLLECTIVE);

    //
    // loop of timesteps
    //
    int k = 0;
    for (k = 0; k < NUM_STEPS; k++)
    {
#ifdef NATIVE_HDF5
#else
        H5VL_adios2_begin_write_step(filename);
#endif
        {
            if (k == 0)
            {
                //
                // empty groups, do not show up in adios
                //
                hid_t gid = H5Gcreate(file_id, "noSuchGroup1", H5P_DEFAULT,
                                      H5P_DEFAULT, H5P_DEFAULT);
                hid_t gid2 = H5Gcreate(gid, "noSuchSubG1", H5P_DEFAULT,
                                       H5P_DEFAULT, H5P_DEFAULT);

                H5Gclose(gid2);
                H5Gclose(gid);
            }
        }

        {
            initIntData(mpi_rank, k, &data, count[0] * count[1]);

            status = H5Dwrite(dset_id, H5T_NATIVE_INT, memspace, filespace,
                              plist_id, data);

            if (mpi_rank != LAZYRANK)
                free(data);
        }

#ifdef NATIVE_HDF5
#else
        H5VL_adios2_endstep(filename);
#endif
        if (0 == mpi_rank)
            printf("...wrote step %d \n", k);

        { // adios2 attributes are global. so only need to write attr of var
          // once
            if (k == 0)
                if (mpi_rank == 0)
                    AttrWriteTests(dset_id);
        }
    } // timestep finished

    H5Dclose(dset_id);
    H5Sclose(filespace);
    H5Sclose(memspace);
    H5Pclose(plist_id);
    H5Fclose(file_id);

#ifdef NATIVE_HDF5
#else
    H5VL_ADIOS2_unset();
#endif
    return 0;
}

void writeScalarVar(hid_t owner_id, const char *varName, hid_t type,
                    void *scalarVal)

{
    hid_t scalarSpace = H5Screate(H5S_SCALAR);
    hid_t scalar_did = H5Dcreate(owner_id, varName, type, scalarSpace,
                                 H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    {
        const char *n = "scalarIntV2";
        htri_t ss = H5Lexists(owner_id, n, H5P_DEFAULT);
        printf("\n... checking: if var %s exists under current group. result: "
               "%d\n",
               n, ss);
    }

    herr_t status = H5Dwrite(scalar_did, type, H5P_DEFAULT, H5P_DEFAULT,
                             H5P_DEFAULT, scalarVal);
    H5Sclose(scalarSpace);
    H5Dclose(scalar_did);
}

void writeArrayVar(hid_t var_id, hsize_t *offset, hsize_t *count,
                   hid_t memspace, int mpi_rank, hid_t plist_id, void *data)
{
    hid_t filespace_id = H5Dget_space(var_id);
    H5Sselect_hyperslab(filespace_id, H5S_SELECT_SET, offset, NULL, count,
                        NULL);

    if (mpi_rank == LAZYRANK)
        H5Sselect_none(filespace_id);

    herr_t status;
    status = H5Dwrite(var_id, H5T_NATIVE_INT, memspace, filespace_id, plist_id,
                      data);

    H5Sclose(filespace_id);
}

int TestW_2(const char *filename, MPI_Comm comm, MPI_Info info, int mpi_size,
            int mpi_rank)
{
    if (mpi_rank == 0)
        printf("\n===> Testing two vars write, NO timesteps, lazy Rank = %d "
               "<=== %s\n",
               LAZYRANK, filename);

    hid_t file_id;
    hid_t file_var1, file_var2;
    hid_t group_var1, group_var2;
    hid_t memspace;
    hsize_t dimsf[2];
    int *data;
    hsize_t count1[2], offset1[2];
    hsize_t count2[1], offset2[1];
    hid_t plist_id;
    int i;
    herr_t status;

    file_id = GetFileID(comm, info, filename, true);
    if (file_id < 0)
        return -1;

    //  file level scalar  var
    int intVal = 919;
    writeScalarVar(file_id, "scalarIntV", H5T_NATIVE_INT, &intVal);
    //  creats group & write group level scalar var and  attr
    hid_t gid =
        H5Gcreate(file_id, GROUPNAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    int intVal2 = 818;
    writeScalarVar(gid, "scalarIntV2", H5T_NATIVE_INT, &intVal2);
    int intVal3 = 111;
    writeScalarAttr(gid, H5T_NATIVE_INT, ATTRNAME_INT, &intVal3);
    writeScalarAttr(gid, H5T_NATIVE_CHAR, ATTRNAME_CHAR, "a");
    H5Gclose(gid);

    // array  data
    dimsf[0] = NX;
    dimsf[1] = NY;
    file_var1 = defineArrayVar(DIM, dimsf, file_id, VARNAME1, H5T_NATIVE_INT);

    gid = H5Gopen(file_id, GROUPNAME, H5P_DEFAULT);
    hid_t gid2 =
        H5Gcreate(gid, SUBGROUPNAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    writeScalarVar(gid2, "scalarIntV3", H5T_NATIVE_INT, &intVal2);
    writeScalarAttr(gid2, H5T_NATIVE_INT, ATTRNAME_INT, &intVal3);
    H5Gclose(gid2);

    group_var1 = defineArrayVar(DIM, dimsf, gid, VARNAME1, H5T_NATIVE_INT);

    hsize_t dims1D[1];
    dims1D[0] = NX * NY;
    file_var2 = defineArrayVar(1, dims1D, file_id, VARNAME2, H5T_NATIVE_INT);

    count1[0] = dimsf[0] / mpi_size;
    count1[1] = dimsf[1];
    offset1[0] = mpi_rank * count1[0];
    offset1[1] = 0;

    count2[0] = dims1D[0] / mpi_size;
    offset2[0] = mpi_rank * count2[0];

    // shape of data
    memspace = H5Screate_simple(DIM, count1, NULL);
    if (mpi_rank == LAZYRANK)
        H5Sselect_none(memspace);

    // data shared by both variables
    initIntData(mpi_rank, 0, &data, count1[0] * count1[1]);

    plist_id = H5Pcreate(H5P_DATASET_XFER);
    H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_COLLECTIVE);

    writeArrayVar(file_var1, offset1, count1, memspace, mpi_rank, plist_id,
                  data);
    writeArrayVar(group_var1, offset1, count1, memspace, mpi_rank, plist_id,
                  data);

    writeArrayVar(file_var2, offset2, count2, memspace, mpi_rank, plist_id,
                  data);

    if (mpi_size == 1)
    {
        printf("TESTING blocked  write\n");
        dims1D[0] = 4;
        hid_t var_id =
            defineArrayVar(1, dims1D, file_id, "testOneD", H5T_NATIVE_INT);
        hsize_t count[1], offset[1];
        count[0] = 1;
        hid_t ms = H5Screate_simple(1, count, NULL);

        int i = 0;
        herr_t status;
        int data[4] = {11, 21, 31, 41};
        for (i = 0; i < 4; i++)
        {
            hid_t filespace_id = H5Dget_space(var_id);
            offset[0] = i;
            H5Sselect_hyperslab(filespace_id, H5S_SELECT_SET, offset, NULL,
                                count, NULL);

            hsize_t n1 = H5Sget_select_npoints(ms);
            hsize_t n2 = H5Sget_select_npoints(filespace_id);

            // status = H5Dwrite(var_id, H5T_NATIVE_INT, ms, filespace_id,
            // plist_id, &(data[i]));
            int val = data[i];
            status = H5Dwrite(var_id, H5T_NATIVE_INT, ms, filespace_id,
                              plist_id, &val);

            H5Sclose(filespace_id);
        }
        H5Dclose(var_id);
    }

    if (mpi_rank != LAZYRANK)
        free(data);

    {
        H5G_info_t info;
        herr_t status = H5Gget_info(gid, &info);
        hsize_t k;
        printf("number of links found in group %s = %llu\n", GROUPNAME,
               info.nlinks);
        for (k = 0; k < info.nlinks; ++k)
        {
            ssize_t name_length = H5Lget_name_by_idx(
                gid, ".", H5_INDEX_NAME, H5_ITER_INC, k, NULL, 0, H5P_DEFAULT);

            if (name_length > 0)
            {
                char name[name_length + 1];
                H5Lget_name_by_idx(gid, ".", H5_INDEX_NAME, H5_ITER_INC, k,
                                   name, 0, H5P_DEFAULT);
                name[name_length] = '\0';
                printf(" \n %llu th link is %s\n", k, name);
            }
        }
    }
    H5Gclose(gid);
    H5Dclose(file_var1);
    H5Dclose(file_var2);
    H5Dclose(group_var1);

    {
        hid_t oid = H5Oopen(file_id, GROUPNAME, H5P_DEFAULT);

        H5O_info_t object_info;
        herr_t ss;
#if H5_VERSION_GE(1, 12, 0)
        ss = H5Oget_info(oid, &object_info, H5O_INFO_NUM_ATTRS);
#else
        ss = H5Oget_info(oid, &object_info);
#endif
        printf(" num of attr found: %llu in %s\n", object_info.num_attrs,
               GROUPNAME);

        {
            int k;
            for (k = 0; k < object_info.num_attrs; k++)
            {
                ssize_t name_length =
                    H5Aget_name_by_idx(oid, ".", H5_INDEX_CRT_ORDER,
                                       H5_ITER_INC, k, NULL, 0, H5P_DEFAULT);

                char name[name_length + 1];
                H5Aget_name_by_idx(oid, ".", H5_INDEX_CRT_ORDER, H5_ITER_INC, k,
                                   name, 0, H5P_DEFAULT);
                name[name_length] = '\0';
                printf(" \n %d th attr is %s\n", k, name);
            }

            ss = H5Lexists(oid, ATTRNAME_INT, H5P_DEFAULT);
            printf("expecting true: %d\n", ss);
            ss = H5Lexists(oid, ATTRNAME_CHAR, H5P_DEFAULT);
            printf("expecting true: %d\n", ss);
            ss = H5Lexists(oid, "scalarIntV2", H5P_DEFAULT);
            printf("expecting true: %d\n", ss);

            if (1 == ss)
                H5Ldelete(oid, "scalarIntV2", H5P_DEFAULT);
        }

        H5Oclose(oid);
    }

    H5Sclose(memspace);
    H5Pclose(plist_id);
    H5Fclose(file_id);

#ifdef NATIVE_HDF5
#else
    H5VL_ADIOS2_unset();
#endif

    return 0;
}

int TestR_1(const char *filename, MPI_Comm comm, MPI_Info info, int mpi_size,
            int mpi_rank)
{
    if (mpi_rank == 0)
        printf("===> Testing read <===  %s\n", filename);

    hid_t fid = GetFileID(comm, info, filename, false);

    if (fid == -1)
        return -1;

    if (mpi_rank == 0)
        AttrReadTests(fid);

    while (true)
    {
#ifdef NATIVE_HDF5
#else
        herr_t status = H5VL_adios2_begin_read_step(filename);
        if (0 != status)
            break;
#endif
        printf("... read in dataset .. %s ", VARNAME1);
        hid_t dataset1 = H5Dopen2(fid, VARNAME1, H5P_DEFAULT);
        if (dataset1 == -1)
            return -1;

        /* set up dimensions of the slab this process accesses */
        hsize_t start[2], count[2], stride[2];

        start[0] = mpi_rank * NX / mpi_size;
        start[1] = 0;
        count[0] = NX / mpi_size;
        count[1] = NY;
        stride[0] = 1;
        stride[1] = 1;

        /* create a file dataspace independently */
        hid_t file_dataspace = H5Dget_space(dataset1);
        if (file_dataspace == -1)
            return -1;

        H5Sselect_hyperslab(file_dataspace, H5S_SELECT_SET, start, stride,
                            count, NULL);

        /* create a memory dataspace independently */
        hid_t mem_dataspace = H5Screate_simple(DIM, count, NULL);

        hid_t xfer_plist = H5P_DEFAULT;
#ifdef MPIO
        xfer_plist = H5Pcreate(H5P_DATASET_XFER);
        assert(xfer_plist != -1);
        ret = H5Pset_dxpl_mpio(xfer_plist, H5FD_MPIO_COLLECTIVE);
        assert(ret != -1);
#endif
        int data_array1[NX / mpi_size][NY]; /* datatype is known as int */
        int i, j;
        for (i = 0; i < NX / mpi_size; i++)
            for (j = 0; j < NY; j++)
                data_array1[i][j] = -1; // init

        /* read data independently or collectively */
        hid_t ret = H5Dread(dataset1, H5T_NATIVE_INT, mem_dataspace,
                            file_dataspace, xfer_plist, data_array1);
        if (ret == -1)
            return -1;

#ifndef PRINTING
        for (i = 0; i < NX / mpi_size; i++)
        {
            printf("rank:%d row:%d(+%llu)[", mpi_rank, i, start[0]);

            for (j = 0; j < NY; j++)
                if ((j < 3) || (j > NY - 3))
                    printf("%d ", data_array1[i][j]);
                else
                    printf(".");

            printf("]\n");
        }
#endif
#ifdef NATIVE_HDF5
#else
        H5VL_adios2_endstep(filename);
#endif
        if (H5P_DEFAULT != xfer_plist)
            H5Pclose(xfer_plist);

        H5Sclose(mem_dataspace);
        H5Sclose(file_dataspace);

        H5Dclose(dataset1);
    } // while
    H5Fclose(fid);

#ifdef NATIVE_HDF5
#else
    H5VL_ADIOS2_unset();
#endif

    return 0;
}
int main(int argc, char **argv)
{
    /*
     * MPI variables
     */
    int mpi_size, mpi_rank;
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Info info = MPI_INFO_NULL;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(comm, &mpi_size);
    MPI_Comm_rank(comm, &mpi_rank);

#ifdef NATIVE_HDF5
    const char *file1Name = "t1.h5";
    const char *file2Name = "t2.h5";
#else
    const char *file1Name = "t1.bp";
    const char *file2Name = "t2.bp";
#endif

    if (argc == 1)
    {
        TestW_1(file1Name, comm, info, mpi_size, mpi_rank);
        TestR_1(file1Name, comm, info, mpi_size, mpi_rank);

        // printf(".. skipping file: %s \n", file2Name);
        TestW_2(file2Name, comm, info, mpi_size, mpi_rank);
        TestR_1(file2Name, comm, info, mpi_size, mpi_rank);
    }
    if (argc > 1)
    {
        if (argv[1][0] == 'w')
            TestW_1(file1Name, comm, info, mpi_size, mpi_rank);
        else if (argv[1][0] == 'r')
            TestR_1(file1Name, comm, info, mpi_size, mpi_rank);
        else
            TestW_2(file2Name, comm, info, mpi_size, mpi_rank);
    }
    MPI_Finalize();
}
