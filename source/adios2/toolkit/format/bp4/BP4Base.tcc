/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP4Base.tcc
 *
 *  Created on: Aug 1, 2018
 *      Author: Lipeng Wan wanl@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP4_BP4BASE_TCC_
#define ADIOS2_TOOLKIT_FORMAT_BP4_BP4BASE_TCC_

#include "BP4Base.h"

#include <algorithm> //std::all_of

#include "adios2/helper/adiosFunctions.h" //NextExponentialSize, helper::CopyFromBuffer

namespace adios2
{
namespace format
{

// PROTECTED

template <class T>
BP4Base::Characteristics<T> BP4Base::ReadElementIndexCharacteristics(
    const std::vector<char> &buffer, size_t &position, const DataTypes dataType,
    const bool untilTimeStep, const bool isLittleEndian) const
{
    Characteristics<T> characteristics;
    characteristics.EntryCount =
        helper::ReadValue<uint8_t>(buffer, position, isLittleEndian);
    characteristics.EntryLength =
        helper::ReadValue<uint32_t>(buffer, position, isLittleEndian);

    ParseCharacteristics(buffer, position, dataType, untilTimeStep,
                         characteristics, isLittleEndian);

    return characteristics;
}

template <>
inline void BP4Base::ParseCharacteristics(
    const std::vector<char> &buffer, size_t &position, const DataTypes dataType,
    const bool untilTimeStep, Characteristics<std::string> &characteristics,
    const bool isLittleEndian) const
{
    const size_t start = position;
    size_t localPosition = 0;

    bool foundTimeStep = false;

    while (localPosition < characteristics.EntryLength)
    {
        const uint8_t id =
            helper::ReadValue<uint8_t>(buffer, position, isLittleEndian);

        switch (id)
        {
        case (characteristic_time_index):
        {
            characteristics.Statistics.Step =
                helper::ReadValue<uint32_t>(buffer, position, isLittleEndian);
            foundTimeStep = true;
            break;
        }

        case (characteristic_file_index):
        {
            characteristics.Statistics.FileIndex =
                helper::ReadValue<uint32_t>(buffer, position, isLittleEndian);
            break;
        }

        case (characteristic_value):
        {
            if (dataType == type_string)
            {
                // first get the length of the string
                characteristics.Statistics.Value =
                    ReadBP4String(buffer, position, isLittleEndian);
                characteristics.Statistics.IsValue = true;
            }
            else if (dataType == type_string_array)
            {
                if (characteristics.Count.size() != 1)
                {
                    // TODO: add exception here?
                    break;
                }

                const size_t elements = characteristics.Count.front();
                characteristics.Statistics.Values.reserve(elements);

                for (size_t e = 0; e < elements; ++e)
                {
                    const size_t length =
                        static_cast<size_t>(helper::ReadValue<uint16_t>(
                            buffer, position, isLittleEndian));

                    characteristics.Statistics.Values.push_back(
                        std::string(&buffer[position], length));

                    position += length;
                }
            }

            break;
        }

        case (characteristic_offset):
        {
            characteristics.Statistics.Offset =
                helper::ReadValue<uint64_t>(buffer, position, isLittleEndian);
            break;
        }

        case (characteristic_payload_offset):
        {
            characteristics.Statistics.PayloadOffset =
                helper::ReadValue<uint64_t>(buffer, position, isLittleEndian);
            break;
        }

        case (characteristic_dimensions):
        {
            auto lf_CheckEmpty = [](const Dims &dimensions) -> bool {
                return std::all_of(
                    dimensions.begin(), dimensions.end(),
                    [](const size_t dimension) { return dimension == 0; });
            };

            const size_t dimensionsSize = static_cast<size_t>(
                helper::ReadValue<uint8_t>(buffer, position, isLittleEndian));

            characteristics.Shape.reserve(dimensionsSize);
            characteristics.Start.reserve(dimensionsSize);
            characteristics.Count.reserve(dimensionsSize);
            position += 2; // skip length (not required)

            for (size_t d = 0; d < dimensionsSize; ++d)
            {
                characteristics.Count.push_back(
                    static_cast<size_t>(helper::ReadValue<uint64_t>(
                        buffer, position, isLittleEndian)));

                characteristics.Shape.push_back(
                    static_cast<size_t>(helper::ReadValue<uint64_t>(
                        buffer, position, isLittleEndian)));

                characteristics.Start.push_back(
                    static_cast<size_t>(helper::ReadValue<uint64_t>(
                        buffer, position, isLittleEndian)));
            }

            // check for local variables
            const bool emptyShape = lf_CheckEmpty(characteristics.Shape);

            // check if it's a local value
            if (!emptyShape && dimensionsSize == 1)
            {
                if (characteristics.Shape.front() == LocalValueDim)
                {
                    characteristics.Start.clear();
                    characteristics.Count.clear();
                    characteristics.EntryShapeID = ShapeID::LocalValue;
                }
            }

            break;
        }
        default:
        {
            throw std::invalid_argument("ERROR: characteristic ID " +
                                        std::to_string(id) +
                                        " not supported\n");
        }

        } // end id switch

        if (untilTimeStep && foundTimeStep)
        {
            break;
        }

        localPosition = position - start;
    }
}

template <class T>
inline void BP4Base::ParseCharacteristics(const std::vector<char> &buffer,
                                          size_t &position,
                                          const DataTypes /*dataType*/,
                                          const bool untilTimeStep,
                                          Characteristics<T> &characteristics,
                                          const bool isLittleEndian) const
{
    const size_t start = position;
    size_t localPosition = 0;

    bool foundTimeStep = false;

    while (localPosition < characteristics.EntryLength)
    {
        const CharacteristicID id = static_cast<CharacteristicID>(
            helper::ReadValue<uint8_t>(buffer, position, isLittleEndian));

        switch (id)
        {
        case (characteristic_time_index):
        {
            characteristics.Statistics.Step =
                helper::ReadValue<uint32_t>(buffer, position, isLittleEndian);
            foundTimeStep = true;
            break;
        }

        case (characteristic_file_index):
        {
            characteristics.Statistics.FileIndex =
                helper::ReadValue<uint32_t>(buffer, position, isLittleEndian);
            break;
        }

        case (characteristic_value):
        {
            // we are relying that count contains the dimensions
            if (characteristics.Count.empty() || characteristics.Count[0] == 1)
            {
                characteristics.Statistics.Value =
                    helper::ReadValue<T>(buffer, position, isLittleEndian);
                characteristics.Statistics.IsValue = true;
                characteristics.EntryShapeID = ShapeID::GlobalValue;
                // adding Min Max for global and local values
                characteristics.Statistics.Min =
                    characteristics.Statistics.Value;
                characteristics.Statistics.Max =
                    characteristics.Statistics.Value;
            }
            else // used for attributes
            {
                const size_t size = characteristics.Count[0];
                characteristics.Statistics.Values.resize(size);
#ifdef ADIOS2_HAVE_ENDIAN_REVERSE

                if (helper::IsLittleEndian() != isLittleEndian)
                {
                    helper::ReverseCopyFromBuffer(
                        buffer, position,
                        characteristics.Statistics.Values.data(), size);
                }
                else
                {
                    helper::CopyFromBuffer(
                        buffer, position,
                        characteristics.Statistics.Values.data(), size);
                }
#else
                helper::CopyFromBuffer(buffer, position,
                                       characteristics.Statistics.Values.data(),
                                       size);
#endif
            }
            break;
        }

        case (characteristic_min):
        {
            characteristics.Statistics.Min =
                helper::ReadValue<T>(buffer, position, isLittleEndian);
            break;
        }

        case (characteristic_max):
        {
            characteristics.Statistics.Max =
                helper::ReadValue<T>(buffer, position, isLittleEndian);
            break;
        }

        case (characteristic_offset):
        {
            characteristics.Statistics.Offset =
                helper::ReadValue<uint64_t>(buffer, position, isLittleEndian);
            break;
        }

        case (characteristic_payload_offset):
        {
            characteristics.Statistics.PayloadOffset =
                helper::ReadValue<uint64_t>(buffer, position, isLittleEndian);
            break;
        }

        case (characteristic_dimensions):
        {
            auto lf_CheckEmpty = [](const Dims &dimensions) -> bool {
                return std::all_of(
                    dimensions.begin(), dimensions.end(),
                    [](const size_t dimension) { return dimension == 0; });
            };

            const size_t dimensionsSize = static_cast<size_t>(
                helper::ReadValue<uint8_t>(buffer, position, isLittleEndian));

            characteristics.Shape.reserve(dimensionsSize);
            characteristics.Start.reserve(dimensionsSize);
            characteristics.Count.reserve(dimensionsSize);
            position += 2; // skip length (not required)

            for (size_t d = 0; d < dimensionsSize; ++d)
            {
                characteristics.Count.push_back(
                    static_cast<size_t>(helper::ReadValue<uint64_t>(
                        buffer, position, isLittleEndian)));

                characteristics.Shape.push_back(
                    static_cast<size_t>(helper::ReadValue<uint64_t>(
                        buffer, position, isLittleEndian)));

                characteristics.Start.push_back(
                    static_cast<size_t>(helper::ReadValue<uint64_t>(
                        buffer, position, isLittleEndian)));
            }
            // check for local variables (Start and Shape must be all zero)
            const bool emptyShape = lf_CheckEmpty(characteristics.Shape);

            // check if it's a local value
            if (!emptyShape && dimensionsSize == 1)
            {
                if (characteristics.Shape.front() == LocalValueDim)
                {
                    characteristics.Start.clear();
                    characteristics.Count.clear();
                    characteristics.EntryShapeID = ShapeID::LocalValue;
                    break;
                }
            }

            const bool emptyStart = lf_CheckEmpty(characteristics.Start);
            const bool emptyCount = lf_CheckEmpty(characteristics.Count);

            if (emptyShape && emptyStart && !emptyCount) // local array
            {
                characteristics.Shape.clear();
                characteristics.Start.clear();
                characteristics.EntryShapeID = ShapeID::LocalArray;
            }
            else if (emptyShape && emptyStart && emptyCount) // global value
            {
                characteristics.Shape.clear();
                characteristics.Start.clear();
                characteristics.Count.clear();
                characteristics.EntryShapeID = ShapeID::GlobalValue;
            }
            else
            {
                // TODO joined dimension
                characteristics.EntryShapeID = ShapeID::GlobalArray;
            }

            break;
        }
        case (characteristic_bitmap):
        {
            characteristics.Statistics.Bitmap = std::bitset<32>(
                helper::ReadValue<uint32_t>(buffer, position, isLittleEndian));
            break;
        }
        case (characteristic_stat):
        {
            if (characteristics.Statistics.Bitmap.none())
            {
                break;
            }

            for (unsigned int i = 0; i <= 6; ++i)
            {
                if (!characteristics.Statistics.Bitmap.test(i))
                {
                    continue;
                }

                const VariableStatistics bitStat =
                    static_cast<VariableStatistics>(i);

                switch (bitStat)
                {
                case (statistic_min):
                {
                    characteristics.Statistics.Min =
                        helper::ReadValue<T>(buffer, position, isLittleEndian);
                    break;
                }
                case (statistic_max):
                {
                    characteristics.Statistics.Max =
                        helper::ReadValue<T>(buffer, position, isLittleEndian);
                    break;
                }
                case (statistic_sum):
                {
                    characteristics.Statistics.BitSum =
                        helper::ReadValue<double>(buffer, position,
                                                  isLittleEndian);
                    break;
                }
                case (statistic_sum_square):
                {
                    characteristics.Statistics.BitSumSquare =
                        helper::ReadValue<double>(buffer, position,
                                                  isLittleEndian);
                    break;
                }
                case (statistic_finite):
                {
                    characteristics.Statistics.BitFinite =
                        helper::ReadValue<uint8_t>(buffer, position,
                                                   isLittleEndian);
                    break;
                }
                case (statistic_hist):
                {
                    throw std::invalid_argument(
                        "ERROR: ADIOS2 BP4 engine doesn't support "
                        "histogram statistics\n");
                }
                case (statistic_cnt):
                {
                    throw std::invalid_argument(
                        "ERROR: ADIOS2 BP4 engine doesn't support "
                        "count statistics\n");
                }

                } // switch
            }     // for
            break;
        }
        case (characteristic_transform_type):
        {
            const size_t typeLength = static_cast<size_t>(
                helper::ReadValue<uint8_t>(buffer, position, isLittleEndian));
            characteristics.Statistics.Op.Type =
                std::string(&buffer[position], typeLength);
            position += typeLength;

            characteristics.Statistics.Op.PreDataType =
                helper::ReadValue<uint8_t>(buffer, position, isLittleEndian);

            const size_t dimensionsSize = static_cast<size_t>(
                helper::ReadValue<uint8_t>(buffer, position, isLittleEndian));

            characteristics.Statistics.Op.PreShape.reserve(dimensionsSize);
            characteristics.Statistics.Op.PreStart.reserve(dimensionsSize);
            characteristics.Statistics.Op.PreCount.reserve(dimensionsSize);
            position += 2; // skip length (not required)

            for (size_t d = 0; d < dimensionsSize; ++d)
            {
                characteristics.Statistics.Op.PreCount.push_back(
                    static_cast<size_t>(helper::ReadValue<uint64_t>(
                        buffer, position, isLittleEndian)));

                characteristics.Statistics.Op.PreShape.push_back(
                    static_cast<size_t>(helper::ReadValue<uint64_t>(
                        buffer, position, isLittleEndian)));

                characteristics.Statistics.Op.PreStart.push_back(
                    static_cast<size_t>(helper::ReadValue<uint64_t>(
                        buffer, position, isLittleEndian)));
            }

            const size_t metadataLength = static_cast<size_t>(
                helper::ReadValue<uint16_t>(buffer, position, isLittleEndian));

            characteristics.Statistics.Op.Metadata =
                std::vector<char>(buffer.begin() + position,
                                  buffer.begin() + position + metadataLength);
            position += metadataLength;

            characteristics.Statistics.Op.IsActive = true;
            break;
        }
        default:
        {
            throw std::invalid_argument("ERROR: characteristic ID " +
                                        std::to_string(id) +
                                        " not supported\n");
        }

        } // end id switch

        if (untilTimeStep && foundTimeStep)
        {
            break;
        }

        localPosition = position - start;
    }
}

template <class T>
std::map<size_t, std::shared_ptr<BP4Operation>> BP4Base::SetBP4Operations(
    const std::vector<core::VariableBase::Operation> &operations) const
{
    std::map<size_t, std::shared_ptr<BP4Operation>> bp4Operations;
    std::shared_ptr<BP4Operation> bp4Operation;

    for (auto i = 0; i < operations.size(); ++i)
    {
        const std::string type = operations[i].Op->m_Type;
        std::shared_ptr<BP4Operation> bp4Operation = SetBP4Operation(type);

        if (bp4Operation) // if the result is a supported type
        {
            bp4Operations.emplace(i, bp4Operation);
        }
    }
    return bp4Operations;
}

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_FORMAT_BP4_BP4Base_TCC_ */
