/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ZfpMan.cpp
 *
 *  Created on: Apr 20, 2017
 *      Author: Jason Wang
 */

#include "ZfpMan.h"

#include <zfp.h>

int ZfpMan::init(json a_jmsg)
{
    if (a_jmsg["compression_rate"].is_number())
    {
        m_compression_rate = a_jmsg["compression_rate"].get<double>();
    }
    return 0;
}

int ZfpMan::put(const void *a_data, json a_jmsg)
{
    put_begin(a_data, a_jmsg);

    std::vector<char> compressed_data;
    if (check_json(a_jmsg, {"doid", "var", "dtype", "putshape"}, "ZfpMan"))
    {
        if (not a_jmsg["compression_rate"].is_number())
        {
            a_jmsg["compression_rate"] = m_compression_rate;
        }
        compress(const_cast<void *>(a_data), compressed_data, a_jmsg);
    }

    put_end(compressed_data.data(), a_jmsg);
    return 0;
}

int ZfpMan::get(void *a_data, json &a_jmsg) { return 0; }

void ZfpMan::flush() { flush_next(); }

int ZfpMan::compress(void *a_input, std::vector<char> &a_output, json &a_jmsg)
{
    std::string dtype = a_jmsg["dtype"];
    std::vector<size_t> shape = a_jmsg["putshape"].get<std::vector<size_t>>();
    int compression_rate = a_jmsg["compression_rate"].get<int>();

    int status = 0; // return value: 0 = success
    uint dim = 1;
    zfp_type type = zfp_type_none; // array scalar type
    zfp_field *field;              // array meta data
    zfp_stream *zfp;               // compressed stream
    size_t bufsize;                // byte size of compressed buffer
    bitstream *stream;             // bit stream to write to or read from
    size_t zfpsize;                // byte size of compressed stream

    // allocate meta data for the 3D array a[nz][ny][nx]
    if (dtype == "int")
    {
        type = zfp_type_int32;
    }
    else if (dtype == "long")
    {
        type = zfp_type_int64;
    }
    else if (dtype == "float")
    {
        type = zfp_type_float;
    }
    else if (dtype == "double")
    {
        type = zfp_type_double;
    }

    switch (shape.size())
    {
    case 3:
        field = zfp_field_3d(a_input, type, shape[0], shape[1], shape[2]);
        dim = 3;
        break;
    case 2:
        field = zfp_field_2d(a_input, type, shape[0], shape[1]);
        dim = 2;
        break;
    case 1:
        field = zfp_field_1d(a_input, type, shape[0]);
        break;
    default:
        field = zfp_field_1d(a_input, type, product(shape));
    }

    // allocate meta data for a compressed stream
    zfp = zfp_stream_open(NULL);

    // set compression mode and parameters via one of three functions
    zfp_stream_set_rate(zfp, compression_rate, type, dim, 0);
    // zfp_stream_set_precision(zfp, m_precision, type);
    // zfp_stream_set_accuracy(zfp, m_accuracy, type);

    // allocate buffer for compressed data
    bufsize = zfp_stream_maximum_size(zfp, field);
    a_output.resize(bufsize);

    // associate bit stream with allocated buffer
    stream = stream_open(a_output.data(), bufsize);
    zfp_stream_set_bit_stream(zfp, stream);
    zfp_stream_rewind(zfp);

    // compress or decompress entire array

    zfpsize = zfp_compress(zfp, field);

    if (!zfpsize)
    {
        logging("ZFP compression failed!");
        status = 1;
    }

    a_jmsg["compressed_size"] = bufsize;
    a_jmsg["compression_method"] = "zfp";

    // clean up
    zfp_field_free(field);
    zfp_stream_close(zfp);
    stream_close(stream);

    return 0;
}

int ZfpMan::decompress(void *a_input, std::vector<char> &a_output, json &a_jmsg)
{

    std::string dtype = a_jmsg["dtype"];
    std::vector<size_t> shape = a_jmsg["putshape"].get<std::vector<size_t>>();
    int compression_rate = a_jmsg["compression_rate"].get<int>();

    int status = 0; // return value: 0 = success
    uint dim = 1;
    zfp_type type = zfp_type_none; // array scalar type
    zfp_field *field;              // array meta data
    zfp_stream *zfp;               // compressed stream
    size_t bufsize = a_jmsg["compressed_size"]
                         .get<size_t>(); // byte size of compressed buffer
    bitstream *stream;                   // bit stream to write to or read from
    size_t zfpsize;                      // byte size of compressed stream

    // allocate meta data for the 3D array a[nz][ny][nx]
    if (dtype == "int")
    {
        type = zfp_type_int32;
    }
    else if (dtype == "long")
    {
        type = zfp_type_int64;
    }
    else if (dtype == "float")
    {
        type = zfp_type_float;
    }
    else if (dtype == "double")
    {
        type = zfp_type_double;
    }

    a_output.resize(product(shape, dsize(dtype)));

    switch (shape.size())
    {
    case 3:
        field =
            zfp_field_3d(a_output.data(), type, shape[0], shape[1], shape[2]);
        dim = 3;
        break;
    case 2:
        field = zfp_field_2d(a_output.data(), type, shape[0], shape[1]);
        dim = 2;
        break;
    case 1:
        field = zfp_field_1d(a_output.data(), type, shape[0]);
        break;
    default:
        field = zfp_field_1d(a_output.data(), type, product(shape));
    }

    zfp = zfp_stream_open(NULL);
    zfp_stream_set_rate(zfp, compression_rate, type, dim, 0);
    stream = stream_open(a_input, bufsize);
    zfp_stream_set_bit_stream(zfp, stream);
    zfp_stream_rewind(zfp);
    if (!zfp_decompress(zfp, field))
    {
        fprintf(stderr, "decompression failed\n");
        status = 1;
    }
    zfp_field_free(field);
    zfp_stream_close(zfp);
    stream_close(stream);

    return 0;
}

void ZfpMan::transform(std::vector<char> &a_data, json &a_jmsg)
{
    std::string dtype = a_jmsg["dtype"];
    std::vector<size_t> shape = a_jmsg["putshape"].get<std::vector<size_t>>();
    int compression_rate = a_jmsg["compression_rate"].get<int>();
    size_t putbytes = a_jmsg["putbytes"].get<size_t>();
    std::vector<char> output(putbytes);

    int status = 0; // return value: 0 = success
    uint dim = 1;
    zfp_type type = zfp_type_none; // array scalar type
    zfp_field *field;              // array meta data
    zfp_stream *zfp;               // compressed stream
    size_t bufsize = a_jmsg["compressed_size"]
                         .get<size_t>(); // byte size of compressed buffer
    bitstream *stream;                   // bit stream to write to or read from
    size_t zfpsize;                      // byte size of compressed stream

    // allocate meta data for the 3D array a[nz][ny][nx]
    if (dtype == "int")
    {
        type = zfp_type_int32;
    }
    else if (dtype == "long")
    {
        type = zfp_type_int64;
    }
    else if (dtype == "float")
    {
        type = zfp_type_float;
    }
    else if (dtype == "double")
    {
        type = zfp_type_double;
    }

    switch (shape.size())
    {
    case 3:
        field = zfp_field_3d(output.data(), type, shape[0], shape[1], shape[2]);
        dim = 3;
        break;
    case 2:
        field = zfp_field_2d(output.data(), type, shape[0], shape[1]);
        dim = 2;
        break;
    case 1:
        field = zfp_field_1d(output.data(), type, shape[0]);
        break;
    default:
        field = zfp_field_1d(output.data(), type, product(shape));
    }

    zfp = zfp_stream_open(NULL);
    zfp_stream_set_rate(zfp, compression_rate, type, dim, 0);
    stream = stream_open(a_data.data(), bufsize);
    zfp_stream_set_bit_stream(zfp, stream);
    zfp_stream_rewind(zfp);
    if (!zfp_decompress(zfp, field))
    {
        fprintf(stderr, "decompression failed\n");
        status = 1;
    }
    zfp_field_free(field);
    zfp_stream_close(zfp);
    stream_close(stream);

    a_data = output;
}
