/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "CompressCAESAR.h"
#include "adios2/helper/adiosFunctions.h"
#include "data_utils.h"
#include "dataset/dataset.h"
#include "models/caesar_compress.h"
#include "models/caesar_decompress.h"
#include <cstring>

namespace adios2
{
namespace core
{
namespace compress
{

template <typename T>
void WriteParameter(char *buffer, size_t &pos, const T &value)
{
    std::memcpy(buffer + pos, &value, sizeof(T));
    pos += sizeof(T);
}

template <typename T>
T ReadParameter(const char *buffer, size_t &pos)
{
    T value;
    std::memcpy(&value, buffer + pos, sizeof(T));
    pos += sizeof(T);
    return value;
}

void WriteString(char *buffer, size_t &pos, const std::string &str)
{
    uint64_t len = str.size();
    WriteParameter(buffer, pos, len);
    std::memcpy(buffer + pos, str.data(), len);
    pos += len;
}

std::string ReadString(const char *buffer, size_t &pos)
{
    uint64_t len = ReadParameter<uint64_t>(buffer, pos);
    std::string str(len, '\0');
    std::memcpy(&str[0], buffer + pos, len);
    pos += len;
    return str;
}

template <typename T>
void WriteVector(char *buffer, size_t &pos, const std::vector<T> &vec)
{
    uint64_t size = vec.size();
    WriteParameter(buffer, pos, size);
    if (size > 0)
    {
        std::memcpy(buffer + pos, vec.data(), size * sizeof(T));
        pos += size * sizeof(T);
    }
}

template <typename T>
std::vector<T> ReadVector(const char *buffer, size_t &pos)
{
    uint64_t size = ReadParameter<uint64_t>(buffer, pos);
    std::vector<T> vec(size);
    if (size > 0)
    {
        std::memcpy(vec.data(), buffer + pos, size * sizeof(T));
        pos += size * sizeof(T);
    }
    return vec;
}

void WriteVectorOfStrings(char *buffer, size_t &pos, const std::vector<std::string> &vec)
{
    uint64_t count = vec.size();
    WriteParameter(buffer, pos, count);
    for (const auto &str : vec)
        WriteString(buffer, pos, str);
}

std::vector<std::string> ReadVectorOfStrings(const char *buffer, size_t &pos)
{
    uint64_t count = ReadParameter<uint64_t>(buffer, pos);
    std::vector<std::string> vec;
    vec.reserve(count);
    for (uint64_t i = 0; i < count; ++i)
        vec.push_back(ReadString(buffer, pos));
    return vec;
}

template <typename T>
void WriteVector2D(char *buffer, size_t &pos, const std::vector<std::vector<T>> &vec2d)
{
    uint64_t outer = vec2d.size();
    WriteParameter(buffer, pos, outer);
    for (const auto &v : vec2d)
        WriteVector(buffer, pos, v);
}

template <typename T>
std::vector<std::vector<T>> ReadVector2D(const char *buffer, size_t &pos)
{
    uint64_t outer = ReadParameter<uint64_t>(buffer, pos);
    std::vector<std::vector<T>> vec2d;
    vec2d.reserve(outer);
    for (uint64_t i = 0; i < outer; ++i)
        vec2d.push_back(ReadVector<T>(buffer, pos));
    return vec2d;
}

template <typename T1, typename T2>
void WritePair(char *buffer, size_t &pos, const std::pair<T1, T2> &p)
{
    WriteParameter(buffer, pos, p.first);
    WriteParameter(buffer, pos, p.second);
}

template <typename T1, typename T2>
std::pair<T1, T2> ReadPair(const char *buffer, size_t &pos)
{
    T1 first = ReadParameter<T1>(buffer, pos);
    T2 second = ReadParameter<T2>(buffer, pos);
    return {first, second};
}

template <typename T1, typename T2>
void WriteVectorOfPairs(char *buffer, size_t &pos, const std::vector<std::pair<T1, T2>> &vec)
{
    uint64_t size = vec.size();
    WriteParameter(buffer, pos, size);
    for (const auto &p : vec)
        WritePair(buffer, pos, p);
}

template <typename T1, typename T2>
std::vector<std::pair<T1, T2>> ReadVectorOfPairs(const char *buffer, size_t &pos)
{
    uint64_t size = ReadParameter<uint64_t>(buffer, pos);
    std::vector<std::pair<T1, T2>> vec;
    vec.reserve(size);
    for (uint64_t i = 0; i < size; ++i)
        vec.push_back(ReadPair<T1, T2>(buffer, pos));
    return vec;
}

void WriteTuple(char *buffer, size_t &pos,
                const std::tuple<int32_t, int32_t, std::vector<int32_t>> &tup)
{
    WriteParameter(buffer, pos, std::get<0>(tup));
    WriteParameter(buffer, pos, std::get<1>(tup));
    WriteVector(buffer, pos, std::get<2>(tup));
}

std::tuple<int32_t, int32_t, std::vector<int32_t>> ReadTuple(const char *buffer, size_t &pos)
{
    int32_t first = ReadParameter<int32_t>(buffer, pos);
    int32_t second = ReadParameter<int32_t>(buffer, pos);
    auto third = ReadVector<int32_t>(buffer, pos);
    return {first, second, third};
}

void WritePaddingInfo(char *buffer, size_t &pos, const PaddingInfo &info)
{
    WriteVector(buffer, pos, info.original_shape);
    WriteParameter(buffer, pos, info.original_length);
    WriteVector(buffer, pos, info.padded_shape);
    WriteParameter(buffer, pos, info.H);
    WriteParameter(buffer, pos, info.W);
}

PaddingInfo ReadPaddingInfo(const char *buffer, size_t &pos)
{
    PaddingInfo info;
    info.original_shape = ReadVector<int64_t>(buffer, pos);
    info.original_length = ReadParameter<int64_t>(buffer, pos);
    info.padded_shape = ReadVector<int64_t>(buffer, pos);
    info.H = ReadParameter<int64_t>(buffer, pos);
    info.W = ReadParameter<int64_t>(buffer, pos);
    return info;
}

void WriteCompressionMetaData(char *buffer, size_t &pos, const CompressionMetaData &meta)
{
    WriteVector(buffer, pos, meta.offsets);
    WriteVector(buffer, pos, meta.scales);
    WriteVector2D(buffer, pos, meta.indexes);
    WriteTuple(buffer, pos, meta.block_info);
    WriteVector(buffer, pos, meta.data_input_shape);
    WriteVectorOfPairs(buffer, pos, meta.filtered_blocks);
    WriteParameter(buffer, pos, meta.global_scale);
    WriteParameter(buffer, pos, meta.global_offset);
    WriteParameter(buffer, pos, meta.pad_T);
    WriteParameter(buffer, pos, meta.all_filtered);
}

CompressionMetaData ReadCompressionMetaData(const char *buffer, size_t &pos)
{
    CompressionMetaData meta;
    meta.offsets = ReadVector<float>(buffer, pos);
    meta.scales = ReadVector<float>(buffer, pos);
    meta.indexes = ReadVector2D<int32_t>(buffer, pos);
    meta.block_info = ReadTuple(buffer, pos);
    meta.data_input_shape = ReadVector<int32_t>(buffer, pos);
    meta.filtered_blocks = ReadVectorOfPairs<int32_t, float>(buffer, pos);
    meta.global_scale = ReadParameter<float>(buffer, pos);
    meta.global_offset = ReadParameter<float>(buffer, pos);
    meta.pad_T = ReadParameter<int64_t>(buffer, pos);
    meta.all_filtered = ReadParameter<bool>(buffer, pos);
    return meta;
}

void WriteGAEMetaData(char *buffer, size_t &pos, const GAEMetaData &meta)
{
    WriteParameter(buffer, pos, meta.GAE_correction_occur);
    WriteVector(buffer, pos, meta.padding_recon_info);
    WriteVector2D(buffer, pos, meta.pcaBasis);
    WriteVector(buffer, pos, meta.uniqueVals);
    WriteParameter(buffer, pos, meta.quanBin);
    WriteParameter(buffer, pos, meta.nVec);
    WriteParameter(buffer, pos, meta.prefixLength);
    WriteParameter(buffer, pos, meta.dataBytes);
    WriteParameter(buffer, pos, meta.coeffIntBytes);
}

GAEMetaData ReadGAEMetaData(const char *buffer, size_t &pos)
{
    GAEMetaData meta;
    meta.GAE_correction_occur = ReadParameter<bool>(buffer, pos);
    meta.padding_recon_info = ReadVector<int>(buffer, pos);
    meta.pcaBasis = ReadVector2D<float>(buffer, pos);
    meta.uniqueVals = ReadVector<float>(buffer, pos);
    meta.quanBin = ReadParameter<double>(buffer, pos);
    meta.nVec = ReadParameter<int64_t>(buffer, pos);
    meta.prefixLength = ReadParameter<int64_t>(buffer, pos);
    meta.dataBytes = ReadParameter<int64_t>(buffer, pos);
    meta.coeffIntBytes = ReadParameter<size_t>(buffer, pos);
    return meta;
}

void WriteLBRCMetaData(char *buffer, size_t &pos, const LBRCMetaData &meta)
{
    WriteParameter(buffer, pos, meta.lbrc_correction_occur);
    WriteParameter(buffer, pos, meta.x_mean);
    WriteParameter(buffer, pos, meta.scale);
    WriteParameter(buffer, pos, meta.block_size);
}

LBRCMetaData ReadLBRCMetaData(const char *buffer, size_t &pos)
{
    LBRCMetaData meta;
    meta.lbrc_correction_occur = ReadParameter<bool>(buffer, pos);
    meta.x_mean = ReadParameter<float>(buffer, pos);
    meta.scale = ReadParameter<float>(buffer, pos);
    meta.block_size = ReadParameter<decltype(meta.block_size)>(buffer, pos);
    return meta;
}

void WriteLBRCBlocks(char *buffer, size_t &pos, const std::vector<LBRCBlock> &blocks)
{
    uint64_t n = blocks.size();
    WriteParameter(buffer, pos, n);
    for (const auto &blk : blocks)
    {
        WriteParameter(buffer, pos, blk.step);
        WriteParameter(buffer, pos, blk.bit_count);
        WriteVector2D(buffer, pos, blk.streams);
    }
}

std::vector<LBRCBlock> ReadLBRCBlocks(const char *buffer, size_t &pos)
{
    uint64_t n = ReadParameter<uint64_t>(buffer, pos);
    std::vector<LBRCBlock> blocks;
    blocks.reserve(n);
    for (uint64_t i = 0; i < n; ++i)
    {
        LBRCBlock blk;
        blk.step = ReadParameter<double>(buffer, pos);
        blk.bit_count = ReadParameter<uint32_t>(buffer, pos);
        blk.streams = ReadVector2D<uint8_t>(buffer, pos);
        blocks.push_back(std::move(blk));
    }
    return blocks;
}

size_t CompressCAESAR::GetEstimatedSize(const size_t ElemCount, const size_t ElemSize,
                                        const size_t ndims, const size_t *dims) const
{
    const size_t inputSize = ElemCount * ElemSize;
    // Base class returns inputSize + 128, which is far too small.
    // CAESAR output is unbounded: encoded latents, GAE PCA basis,
    // and LBRC streams are all variable-length and can exceed raw input.
    return inputSize * 4 + 32ULL * 1024 * 1024;
}

CompressCAESAR::CompressCAESAR(const Params &parameters)
: Operator("caesar", COMPRESS_CAESAR, "compress", parameters)
{
}

size_t CompressCAESAR::Operate(const char *dataIn, const Dims &blockStart, const Dims &blockCount,
                               const DataType type, char *bufferOut)
{
    const uint8_t bufferVersion = 1;
    size_t bufferOutOffset = 0;

    MakeCommonHeader(bufferOut, bufferOutOffset, bufferVersion);

    const size_t ndims = blockCount.size();
    WriteParameter(bufferOut, bufferOutOffset, ndims);
    for (const auto &d : blockCount)
        WriteParameter(bufferOut, bufferOutOffset, d);
    WriteParameter(bufferOut, bufferOutOffset, type);

    size_t sizeOut = helper::GetTotalSize(blockCount, helper::GetDataTypeSize(type));

    size_t thresholdSize = 1000000;
    auto itThreshold = m_Parameters.find("threshold");
    if (itThreshold != m_Parameters.end())
        thresholdSize = std::stod(itThreshold->second);

    if (sizeOut < thresholdSize)
    {
        WriteParameter(bufferOut, bufferOutOffset, false);
        std::memcpy(bufferOut + bufferOutOffset, dataIn, sizeOut);
        bufferOutOffset += sizeOut;
        return bufferOutOffset;
    }

    torch::Tensor data_tensor;
    std::vector<int64_t> sizes(blockCount.begin(), blockCount.end());

    if (type == DataType::Float)
    {
        data_tensor = torch::from_blob(const_cast<char *>(dataIn), sizes, torch::kFloat32).clone();
    }
    else if (type == DataType::Double)
    {
        data_tensor = torch::from_blob(const_cast<char *>(dataIn), sizes, torch::kFloat64)
                          .to(torch::kFloat32);
    }
    else
    {
        helper::Throw<std::invalid_argument>("Operator", "CompressCAESAR", "Operate",
                                             "Unsupported data type");
    }

    auto model_type = m_Parameters.find("model");
    if (model_type != m_Parameters.end())
    {
        std::string model_name = helper::LowerCase(model_type->second);
        if (model_name != "caesar_v")
        {
            helper::Throw<std::invalid_argument>("Operator", "CompressCAESAR", "Operate",
                                                 "Unknown model type: " + model_name);
        }
    }
    else
    {
        helper::Throw<std::invalid_argument>("Operator", "CompressCAESAR", "Operate",
                                             "Model type not specified");
    }

    std::vector<int64_t> original_shape_vec;
    for (int64_t i = 0; i < data_tensor.dim(); ++i)
        original_shape_vec.push_back(data_tensor.size(i));

    int64_t H, W;
    bool force_padding = false;

    //  a DIM of 1 is not a real dim needs to pad
    if (original_shape_vec.size() >= 5 && original_shape_vec[original_shape_vec.size() - 2] >= 2 &&
        original_shape_vec[original_shape_vec.size() - 1] >= 2)
    {
        H = static_cast<int64_t>(original_shape_vec[original_shape_vec.size() - 2]);
        W = static_cast<int64_t>(original_shape_vec[original_shape_vec.size() - 1]);
    }
    else
    {
        H = 256;
        W = 256;
    }

    PaddingInfo padding_info;
    torch::Tensor padded_5d;
    std::tie(padded_5d, padding_info) = to_5d_and_pad(data_tensor, H, W, force_padding);
    if (!padding_info.was_padded)
        padded_5d = padded_5d.contiguous();
    data_tensor = torch::Tensor();

    DatasetConfig config;
    config.memory_data = padded_5d;
    config.n_frame = 8;
    config.dataset_name = "ADIOS2_Block";
    config.variable_idx = 0;
    config.train_mode = false;
    config.inst_norm = true;
    config.norm_type = "mean_range";
    config.n_overlap = 0;

    int batch_size = 128;

    float accuracy = 0.001f;
    auto itRelEB = m_Parameters.find("accuracy");
    if (itRelEB != m_Parameters.end())
        accuracy = std::stof(itRelEB->second);

    torch::Device device = select_model_device();

    Compressor compressor(device);
    CompressionResult comp = compressor.compress(config, batch_size, accuracy);
    padded_5d = torch::Tensor();

    WriteParameter(bufferOut, bufferOutOffset, true);
    WriteString(bufferOut, bufferOutOffset, get_model_name());
    WriteParameter(bufferOut, bufferOutOffset, comp.use_lbrc);
    WriteVectorOfStrings(bufferOut, bufferOutOffset, comp.encoded_latents);
    WriteVectorOfStrings(bufferOut, bufferOutOffset, comp.encoded_hyper_latents);
    WriteVector(bufferOut, bufferOutOffset, comp.gae_comp_data);
    WriteCompressionMetaData(bufferOut, bufferOutOffset, comp.compressionMetaData);
    WriteGAEMetaData(bufferOut, bufferOutOffset, comp.gaeMetaData);
    WriteParameter(bufferOut, bufferOutOffset, batch_size);
    WriteParameter(bufferOut, bufferOutOffset, config.n_frame);
    WriteLBRCMetaData(bufferOut, bufferOutOffset, comp.lbrcMetaData);
    WriteLBRCBlocks(bufferOut, bufferOutOffset, comp.lbrc_blocks);
    WritePaddingInfo(bufferOut, bufferOutOffset, padding_info);

    return bufferOutOffset;
}

size_t CompressCAESAR::InverseOperate(const char *bufferIn, const size_t sizeIn, char *dataOut)
{
    size_t bufferInOffset = 0;

    const uint8_t operatorType = GetParameter<uint8_t>(bufferIn, bufferInOffset);
    const uint8_t bufferVersion = GetParameter<uint8_t>(bufferIn, bufferInOffset);
    const uint16_t pad = GetParameter<uint16_t>(bufferIn, bufferInOffset);

    if (bufferVersion == 1)
    {
        return DecompressV1(bufferIn + bufferInOffset, sizeIn - bufferInOffset, dataOut);
    }
    else
    {
        helper::Throw<std::runtime_error>("Operator", "CompressCAESAR", "InverseOperate",
                                          "Unknown buffer version: " +
                                              std::to_string(bufferVersion));
    }
    return 0;
}

size_t CompressCAESAR::DecompressV1(const char *bufferIn, const size_t sizeIn, char *dataOut)
{
    size_t bufferInOffset = 0;

    const size_t ndims = ReadParameter<size_t>(bufferIn, bufferInOffset);
    Dims blockCount(ndims);
    for (size_t i = 0; i < ndims; ++i)
        blockCount[i] = ReadParameter<size_t>(bufferIn, bufferInOffset);

    const DataType type = ReadParameter<DataType>(bufferIn, bufferInOffset);
    const bool isCompressed = ReadParameter<bool>(bufferIn, bufferInOffset);
    size_t sizeOut = helper::GetTotalSize(blockCount, helper::GetDataTypeSize(type));

    if (!isCompressed)
    {
        std::memcpy(dataOut, bufferIn + bufferInOffset, sizeOut);
        return sizeOut;
    }

    std::string compressed_model = ReadString(bufferIn, bufferInOffset);
    std::string installed_model = get_model_name();
    if (compressed_model != installed_model)
    {
        helper::Throw<std::runtime_error>("Operator", "CompressCAESAR", "DecompressV1",
                                          "Model mismatch: data compressed with '" +
                                              compressed_model + "' but installed model is '" +
                                              installed_model + "'");
    }

    CompressionResult comp;
    comp.use_lbrc = ReadParameter<bool>(bufferIn, bufferInOffset);
    comp.encoded_latents = ReadVectorOfStrings(bufferIn, bufferInOffset);
    comp.encoded_hyper_latents = ReadVectorOfStrings(bufferIn, bufferInOffset);
    comp.gae_comp_data = ReadVector<uint8_t>(bufferIn, bufferInOffset);
    comp.compressionMetaData = ReadCompressionMetaData(bufferIn, bufferInOffset);
    comp.gaeMetaData = ReadGAEMetaData(bufferIn, bufferInOffset);
    int batch_size = ReadParameter<int>(bufferIn, bufferInOffset);
    int n_frame = ReadParameter<int>(bufferIn, bufferInOffset);
    comp.lbrcMetaData = ReadLBRCMetaData(bufferIn, bufferInOffset);
    comp.lbrc_blocks = ReadLBRCBlocks(bufferIn, bufferInOffset);
    PaddingInfo padding_info = ReadPaddingInfo(bufferIn, bufferInOffset);

    torch::Device device = select_model_device();

    Decompressor decompressor(device);
    torch::Tensor reconstructed = decompressor.decompress(batch_size, n_frame, comp);

    torch::Tensor restored = restore_from_5d(reconstructed, padding_info);
    restored = restored.to(torch::kCPU).contiguous();

    if (type == DataType::Float)
        std::memcpy(dataOut, restored.data_ptr<float>(), sizeOut);
    else if (type == DataType::Double)
    {
        torch::Tensor double_tensor = restored.to(torch::kFloat64);
        std::memcpy(dataOut, double_tensor.data_ptr<double>(), sizeOut);
    }

    return sizeOut;
}

bool CompressCAESAR::IsDataTypeValid(const DataType type) const
{
    return (type == DataType::Float || type == DataType::Double);
}

} // end namespace compress
} // end namespace core
} // end namespace adios2
