/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "CompressCAESAR.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "adios2/helper/adiosFunctions.h"
#include "models/caesar_compress.h"
#include "models/caesar_decompress.h"
#include "models/serialization.h"

namespace adios2
{
namespace core
{
namespace compress
{
namespace
{
void Require(const size_t size, const size_t pos, const size_t count)
{
    if (pos > size || count > size - pos)
        throw std::runtime_error("Truncated CAESAR operator buffer");
}
template <typename T>
void WriteParameter(char *buffer, size_t &pos, const T &value)
{
    std::memcpy(buffer + pos, &value, sizeof(T));
    pos += sizeof(T);
}
template <typename T>
T ReadParameter(const char *buffer, size_t size, size_t &pos)
{
    Require(size, pos, sizeof(T));
    T value;
    std::memcpy(&value, buffer + pos, sizeof(T));
    pos += sizeof(T);
    return value;
}
bool ReadFlag(const char *buffer, size_t size, size_t &pos)
{
    const auto value = ReadParameter<uint8_t>(buffer, size, pos);
    if (value > 1)
        throw std::runtime_error("Invalid CAESAR operator flag");
    return value != 0;
}
size_t CheckedAdd(size_t a, size_t b)
{
    if (b > std::numeric_limits<size_t>::max() - a)
        throw std::overflow_error("CAESAR operator size overflow");
    return a + b;
}
size_t CheckedMultiply(size_t a, size_t b)
{
    if (a && b > std::numeric_limits<size_t>::max() / a)
        throw std::overflow_error("CAESAR operator size overflow");
    return a * b;
}
size_t DataSize(const Dims &dims, DataType type)
{
    if (type != DataType::Float && type != DataType::Double)
        throw std::invalid_argument("CAESAR supports only float and double");
    size_t size = helper::GetDataTypeSize(type);
    for (auto d : dims)
    {
        if (!d || d > size_t(std::numeric_limits<int64_t>::max()))
            throw std::invalid_argument("Invalid CAESAR input dimension");
        size = CheckedMultiply(size, d);
    }
    return size;
}
void ReadHeader(const char *buffer, size_t size, size_t &pos, Dims &dims, DataType &type,
                bool &compressed)
{
    const auto ndims = ReadParameter<size_t>(buffer, size, pos);
    if (!ndims || ndims > (size - pos) / sizeof(size_t))
        throw std::runtime_error("Invalid CAESAR operator dimension count");
    dims.resize(ndims);
    for (auto &d : dims)
        d = ReadParameter<size_t>(buffer, size, pos);
    type = ReadParameter<DataType>(buffer, size, pos);
    compressed = ReadFlag(buffer, size, pos);
}
std::vector<uint8_t> CompressSlice(const torch::Tensor &slice, float accuracy,
                                   caesar::CorrectionMethod method)
{
    CompressionConfig config;
    config.memory_data = slice;
    config.n_frame = 8;
    config.correction_method = method;
    Compressor compressor;
    return caesar::serialize(compressor.compress(config, accuracy));
}
torch::Tensor DecompressSlice(const char *buffer, size_t size, size_t &pos,
                              const std::vector<int64_t> &expectedShape)
{
    const auto length = ReadParameter<uint64_t>(buffer, size, pos);
    if (length > size - pos)
        throw std::runtime_error("Truncated CAESAR slice");
    auto comp = caesar::deserialize(buffer + pos, size_t(length));
    pos += size_t(length);
    if (comp.original_shape != expectedShape)
        throw std::runtime_error("CAESAR slice shape does not match ADIOS header");
    Decompressor decompressor;
    auto restored = decompressor.decompress(comp);
    if (restored.sizes().vec() != expectedShape)
        throw std::runtime_error("Unexpected CAESAR reconstructed shape");
    return restored;
}

struct LegacyReader
{
    const char *data;
    size_t size;
    size_t &pos;
    template <class... T>
    void operator()(T &...values)
    {
        (get(values), ...);
    }
    template <class T>
    void get(T &value)
    {
        static_assert(std::is_arithmetic_v<T>);
        if constexpr (std::is_same_v<T, bool>)
            value = ReadFlag(data, size, pos);
        else
            value = ReadParameter<T>(data, size, pos);
    }
    size_t count()
    {
        auto n = ReadParameter<uint64_t>(data, size, pos);
        if (n > size - pos)
            throw std::runtime_error("Invalid CAESAR V1 length");
        return size_t(n);
    }
    void get(std::string &value)
    {
        const auto n = count();
        Require(size, pos, n);
        value.assign(data + pos, n);
        pos += n;
    }
    template <class T>
    void get(std::vector<T> &value)
    {
        const auto n = count();
        if constexpr (std::is_arithmetic_v<T>)
            Require(size, pos, CheckedMultiply(n, sizeof(T)));
        for (size_t i = 0; i < n; ++i)
        {
            T item{};
            get(item);
            value.push_back(std::move(item));
        }
    }
    template <class T, size_t N>
    void get(std::array<T, N> &value)
    {
        for (auto &item : value)
            get(item);
    }
    template <class X, class Y>
    void get(std::pair<X, Y> &value)
    {
        (*this)(value.first, value.second);
    }
    template <class... T>
    void get(std::tuple<T...> &value)
    {
        std::apply([this](auto &...items) { (*this)(items...); }, value);
    }
};
torch::Tensor DecompressLegacySlice(const char *buffer, size_t size, size_t &pos,
                                    const std::vector<int64_t> &expectedShape)
{
    CompressionResult comp{};
    LegacyReader read{buffer, size, pos};
    read(comp.model_id);
    comp.correction_method =
        caesar::correction_method_from_byte(ReadParameter<uint8_t>(buffer, size, pos));
    read(comp.encoded_latents, comp.encoded_hyper_latents, comp.gae_comp_data);
    auto &m = comp.compressionMetaData;
    read(m.offsets, m.scales, m.indexes, m.block_info, m.data_input_shape, m.filtered_blocks,
         m.global_scale, m.global_offset, m.pad_T, m.all_filtered);
    auto &g = comp.gaeMetaData;
    read(g.GAE_correction_occur, g.padding_recon_info, g.pcaBasis, g.uniqueVals, g.quanBin, g.nVec,
         g.prefixLength, g.dataBytes, g.coeffIntBytes, comp.n_frame);
    auto &l = comp.lbrcMetaData;
    read(l.lbrc_correction_occur, l.x_mean, l.scale, l.block_size);
    const auto blocks = read.count();
    for (size_t i = 0; i < blocks; ++i)
    {
        LBRCBlock block{};
        read(block.step, block.bit_count, block.streams);
        comp.lbrc_blocks.push_back(std::move(block));
    }
    if (comp.correction_method == caesar::CorrectionMethod::NGLR)
    {
        auto &n = comp.nglrMetaData;
        auto &q = n.quantization;
        read(n.schema_version, n.correction_occurred, n.constant_input, q.x_mean, q.scale, q.step,
             q.q_context_scale, q.delta_scale, q.block_t, q.block_h, q.block_w, n.hidden,
             n.q_hidden, n.model_blocks, n.shape);
        const auto weights = read.count();
        for (size_t i = 0; i < weights; ++i)
        {
            nglr::NGLRWeight weight{};
            read(weight.name, weight.shape, weight.values);
            n.weights.push_back(std::move(weight));
        }
        read(comp.nglr_comp_data);
    }
    auto &p = comp.shape_info;
    read(comp.original_shape, p.original_shape, p.original_length, p.padded_shape, p.H, p.W);
    if (comp.original_shape != expectedShape)
        throw std::runtime_error("CAESAR V1 slice shape does not match ADIOS header");
    Decompressor decompressor;
    auto restored = decompressor.decompress(comp);
    if (restored.sizes().vec() != expectedShape)
        throw std::runtime_error("Unexpected CAESAR V1 reconstructed shape");
    return restored;
}
} // namespace

CompressCAESAR::CompressCAESAR(const Params &parameters)
: Operator("caesar", COMPRESS_CAESAR, "compress", parameters)
{
}

size_t CompressCAESAR::Operate(const char *dataIn, const Dims &blockStart, const Dims &blockCount,
                               DataType type, char *bufferOut)
{
    const auto rawSize = DataSize(blockCount, type);
    CheckedAdd(rawSize, 128);
    const auto capacity =
        GetEstimatedSize(rawSize / helper::GetDataTypeSize(type), helper::GetDataTypeSize(type),
                         blockCount.size(), blockCount.data());
    const auto headerSize = CheckedAdd(4 + sizeof(size_t) + sizeof(DataType) + 1,
                                       CheckedMultiply(blockCount.size(), sizeof(size_t)));
    if (CheckedAdd(headerSize, rawSize) > capacity)
        throw std::runtime_error("CAESAR outer header exceeds ADIOS output allocation");
    size_t threshold = 1000000;
    auto parameter = m_Parameters.find("threshold");
    if (parameter != m_Parameters.end())
    {
        const double value = std::stod(parameter->second);
        if (!std::isfinite(value) || value < 0 ||
            value >= double(std::numeric_limits<size_t>::max()))
            throw std::invalid_argument("Invalid CAESAR threshold");
        threshold = size_t(value);
    }
    size_t pos = 0;
    MakeCommonHeader(bufferOut, pos, 2);
    WriteParameter(bufferOut, pos, blockCount.size());
    for (auto d : blockCount)
        WriteParameter(bufferOut, pos, d);
    WriteParameter(bufferOut, pos, type);
    const auto flagPos = pos;
    WriteParameter(bufferOut, pos, uint8_t(1));
    const auto payloadStart = pos;
    auto writeRaw = [&]() {
        bufferOut[flagPos] = 0;
        std::memcpy(bufferOut + payloadStart, dataIn, rawSize);
        return payloadStart + rawSize;
    };
    if (rawSize < threshold)
        return writeRaw();
    if (blockCount.size() < 3 || blockCount.size() > 5)
        throw std::invalid_argument("CAESAR compression requires 3 to 5 dimensions");

    float accuracy = 0.001f;
    parameter = m_Parameters.find("accuracy");
    if (parameter != m_Parameters.end())
        accuracy = std::stof(parameter->second);
    if (!std::isfinite(accuracy) || accuracy <= 0)
        throw std::invalid_argument("CAESAR accuracy must be positive and finite");
    auto method = caesar::CorrectionMethod::GAE;
    parameter = m_Parameters.find("method");
    if (parameter != m_Parameters.end())
        method = caesar::correction_method_from_string(helper::LowerCase(parameter->second));
    const bool fiveD = blockCount.size() == 5;
    const size_t count = fiveD ? blockCount[0] : 1;
    if (1 + (fiveD ? sizeof(uint64_t) : 0) > capacity - pos)
        return writeRaw();
    WriteParameter(bufferOut, pos, uint8_t(fiveD));
    if (fiveD)
        WriteParameter(bufferOut, pos, uint64_t(count));
    const std::vector<int64_t> shape(blockCount.begin(), blockCount.end());
    // Borrow the ADIOS input. CAESAR performs float conversion on each selected
    // slice, avoiding a float32 copy of the entire 5D double-precision field.
    auto data = torch::from_blob(const_cast<char *>(dataIn), shape,
                                 type == DataType::Float ? torch::kFloat32 : torch::kFloat64);
    for (size_t i = 0; i < count; ++i)
    {
        auto slice = fiveD ? data.select(0, i).unsqueeze(0) : data;
        auto bytes = CompressSlice(slice, accuracy, method);
        const auto required = CheckedAdd(sizeof(uint64_t), bytes.size());
        if (required > capacity - pos)
            return writeRaw();
        WriteParameter(bufferOut, pos, uint64_t(bytes.size()));
        std::memcpy(bufferOut + pos, bytes.data(), bytes.size());
        pos += bytes.size();
        // bytes is destroyed here; serialized slices do not accumulate.
    }
    return pos;
}

namespace
{
size_t DecompressSerialized(const char *buffer, size_t size, char *output)
{
    size_t pos = 0;
    Dims dims;
    DataType type;
    bool compressed;
    ReadHeader(buffer, size, pos, dims, type, compressed);
    const auto rawSize = DataSize(dims, type);
    if (!compressed)
    {
        Require(size, pos, rawSize);
        if (rawSize != size - pos)
            throw std::runtime_error("Trailing CAESAR raw bytes");
        std::memcpy(output, buffer + pos, rawSize);
        return rawSize;
    }
    if (dims.size() < 3 || dims.size() > 5)
        throw std::runtime_error("Invalid CAESAR compressed rank");
    const bool fiveD = ReadFlag(buffer, size, pos);
    if (fiveD != (dims.size() == 5))
        throw std::runtime_error("Invalid CAESAR slice mode");
    const auto count = fiveD ? ReadParameter<uint64_t>(buffer, size, pos) : 1;
    if (fiveD && count != dims[0])
        throw std::runtime_error("Invalid CAESAR slice count");
    std::vector<int64_t> expected(dims.begin(), dims.end());
    if (fiveD)
        expected[0] = 1;
    const size_t sliceBytes = fiveD ? rawSize / dims[0] : rawSize;
    for (uint64_t i = 0; i < count; ++i)
    {
        auto restored =
            DecompressSlice(buffer, size, pos, expected); // Convert/copy directly into the
                                                          // caller's allocation. No full float64
        // temporary or intermediate CPU output buffer is needed.
        auto destination =
            torch::from_blob(output + size_t(i) * sliceBytes, expected,
                             type == DataType::Double ? torch::kFloat64 : torch::kFloat32);
        destination.copy_(restored);
    }
    if (pos != size)
        throw std::runtime_error("Trailing CAESAR operator bytes");
    return rawSize;
}

} // namespace

size_t CompressCAESAR::DecompressV1(const char *buffer, size_t size, char *output)
{
    size_t pos = 0;
    Dims dims;
    DataType type;
    bool compressed;
    ReadHeader(buffer, size, pos, dims, type, compressed);
    const auto rawSize = DataSize(dims, type);
    if (!compressed)
    {
        Require(size, pos, rawSize);
        if (rawSize != size - pos)
            throw std::runtime_error("Trailing CAESAR raw bytes");
        std::memcpy(output, buffer + pos, rawSize);
        return rawSize;
    }
    if (dims.size() < 3 || dims.size() > 5)
        throw std::runtime_error("Invalid CAESAR compressed rank");
    const bool fiveD = ReadFlag(buffer, size, pos);
    if (fiveD != (dims.size() == 5))
        throw std::runtime_error("Invalid CAESAR slice mode");
    const auto count = fiveD ? ReadParameter<uint64_t>(buffer, size, pos) : 1;
    if (fiveD && count != dims[0])
        throw std::runtime_error("Invalid CAESAR slice count");
    std::vector<int64_t> expected(dims.begin(), dims.end());
    if (fiveD)
        expected[0] = 1;
    const size_t sliceBytes = fiveD ? rawSize / dims[0] : rawSize;
    for (uint64_t i = 0; i < count; ++i)
    {
        auto restored = DecompressLegacySlice(buffer, size, pos,
                                              expected); // Convert/copy directly into the
                                                         // caller's allocation. No full float64
        // temporary or intermediate CPU output buffer is needed.
        auto destination =
            torch::from_blob(output + size_t(i) * sliceBytes, expected,
                             type == DataType::Double ? torch::kFloat64 : torch::kFloat32);
        destination.copy_(restored);
    }
    if (pos != size)
        throw std::runtime_error("Trailing CAESAR operator bytes");
    return rawSize;
}

size_t CompressCAESAR::InverseOperate(const char *buffer, size_t size, char *output)
{
    Require(size, 0, 4);
    size_t pos = 0;
    GetParameter<uint8_t>(buffer, pos);
    const auto version = GetParameter<uint8_t>(buffer, pos);
    GetParameter<uint16_t>(buffer, pos);
    if (version == 2)
        return DecompressSerialized(buffer + pos, size - pos, output);
    if (version == 1)
        return DecompressV1(buffer + pos, size - pos, output);
    throw std::runtime_error("Unknown CAESAR operator buffer version: " + std::to_string(version));
}
bool CompressCAESAR::IsDataTypeValid(DataType type) const
{
    return type == DataType::Float || type == DataType::Double;
}
} // namespace compress
} // namespace core
} // namespace adios2
