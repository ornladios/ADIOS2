//
// Created by cguo51 on 7/27/23.
//

#ifndef ADIOS2_KVCACHE_QUERYBOX_H
#define ADIOS2_KVCACHE_QUERYBOX_H

#include <adios2/helper/adiosType.h>
#include <cstring>
#include <iostream>
#include <set>
#include <vector>

namespace adios2
{

namespace kvcache
{

// QueryBox is a class to represent a box in a multi-dimensional space
class QueryBox
{
public:
    helper::DimsArray Start;
    helper::DimsArray Count;
    size_t DimCount;

    QueryBox() : Start(helper::MAX_DIMS), Count(helper::MAX_DIMS), DimCount(helper::MAX_DIMS) {}

    explicit QueryBox(size_t dimCount) : Start(dimCount), Count(dimCount)
    {
        DimCount = dimCount;
        for (size_t i = 0; i < DimCount; ++i)
        {
            Start[i] = 0;
            Count[i] = 0;
        }
    }

    QueryBox(const helper::DimsArray &start, const helper::DimsArray &count)
    : Start(start.size()), Count(count.size())
    {
        DimCount = start.size();
        for (size_t i = 0; i < DimCount; ++i)
        {
            Start[i] = start[i];
            Count[i] = count[i];
        }
    }

    QueryBox(const QueryBox &box) : Start(box.Start.size()), Count(box.Count.size())
    {
        DimCount = box.DimCount;
        for (size_t i = 0; i < DimCount; ++i)
        {
            Start[i] = box.Start[i];
            Count[i] = box.Count[i];
        }
    }

    // Constructor with key string
    explicit QueryBox(const std::string &key)
    : Start(helper::MAX_DIMS), Count(helper::MAX_DIMS), DimCount(helper::MAX_DIMS)
    {
        // Lambda function to extract dimensions from key string
        auto lf_ExtractDimensions = [](const std::string &key, const std::string &delimiter,
                                       std::string &dimStr) {
            size_t pos = key.find(delimiter);
            if (pos == std::string::npos)
            {
                throw std::invalid_argument("Delimiter not found in key");
            }
            size_t end = key.find("|", pos + delimiter.length());
            if (end == std::string::npos)
            {
                throw std::invalid_argument("End delimiter not found in key");
            }
            dimStr = key.substr(pos + delimiter.length(), end - pos - delimiter.length());
        };

        auto lf_ExtractBox = [](const std::string &dimStr, helper::DimsArray &data) {
            std::istringstream dimStream(dimStr);
            std::string token;
            size_t i = 0;
            while (std::getline(dimStream, token, '_'))
            {
                data[i] = std::stoul(token);
                i++;
            }
        };

        std::string startDimStr;
        std::string countDimStr;

        lf_ExtractDimensions(key, "|Start_", startDimStr);
        lf_ExtractDimensions(key, "|Count_", countDimStr);

        DimCount = std::count(startDimStr.begin(), startDimStr.end(), '_') + 1;

        lf_ExtractBox(startDimStr, Start);
        lf_ExtractBox(countDimStr, Count);
    }

    // size
    size_t size() const
    {
        size_t s = 1;
        for (size_t i = 0; i < DimCount; ++i)
        {
            s *= Count[i];
        }
        return s;
    }

    // ToString
    std::string toString() const
    {
        std::string str = "|Start_";
        for (size_t i = 0; i < DimCount; ++i)
        {
            str += std::to_string(Start[i]);
            if (i != DimCount - 1)
            {
                str += "_";
            }
        }
        str += "|Count_";
        for (size_t i = 0; i < DimCount; ++i)
        {
            str += std::to_string(Count[i]);
            if (i != DimCount - 1)
            {
                str += "_";
            }
        }
        str += "|";
        return str;
    }

    // determine if a box is equal to another box
    bool operator==(const QueryBox &box) const
    {
        return std::strcmp(toString().c_str(), box.toString().c_str()) == 0;
    }

    // Assignment operator
    QueryBox &operator=(const QueryBox &box)
    {
        if (this == &box)
        {
            return *this; // handle self-assignment
        }

        // Copy the size
        DimCount = box.DimCount;

        // Copy elements
        for (size_t i = 0; i < DimCount; ++i)
        {
            Start[i] = box.Start[i];
            Count[i] = box.Count[i];
        }

        return *this;
    }

    // convert helper::DimsArray to std::vector<size_t>
    void StartToVector(std::vector<size_t> &vec) const
    {
        for (size_t i = 0; i < DimCount; ++i)
        {
            vec.push_back(Start[i]);
        }
    }

    void CountToVector(std::vector<size_t> &vec) const
    {
        for (size_t i = 0; i < DimCount; ++i)
        {
            vec.push_back(Count[i]);
        }
    }

    // check if *this is interacted in another box, return the new intersection box pointer
    bool IsInteracted(const QueryBox &box, QueryBox &intersection) const
    {
        if (DimCount != box.DimCount)
        {
            return false;
        }

        for (size_t i = 0; i < DimCount; ++i)
        {
            if (Start[i] > box.Start[i] + box.Count[i] || box.Start[i] > Start[i] + Count[i])
            {
                return false;
            }
        }

        for (size_t i = 0; i < DimCount; ++i)
        {
            intersection.Start[i] = std::max(Start[i], box.Start[i]);
            intersection.Count[i] =
                std::min(Start[i] + Count[i], box.Start[i] + box.Count[i]) - intersection.Start[i];
        }
        return true;
    }

    // cut *this from big box, return a list of regular boxes
    // Note: *this must be fully contained by big box
    void NdCut(const QueryBox &bigBox, std::vector<QueryBox> &regularBoxes)
    {
        if (bigBox == *this)
        {
            return;
        }

        // find the cut dimension with the biggest size
        size_t maxCutDimSize = 0;
        QueryBox maxCutDimBox(DimCount);
        for (size_t i = 0; i < DimCount; ++i)
        {
            // if the start and count are the same, means no cut in this dimension, skip
            if (Start[i] == bigBox.Start[i] && Count[i] == bigBox.Count[i])
            {
                continue;
            }
            else
            {
                if (Start[i] != bigBox.Start[i])
                {
                    size_t cutDimDiff = Start[i] - bigBox.Start[i];
                    size_t cutDimSize = bigBox.size() / bigBox.Count[i] * cutDimDiff;
                    if (cutDimSize > maxCutDimSize)
                    {
                        maxCutDimSize = cutDimSize;
                        maxCutDimBox = bigBox;
                        maxCutDimBox.Count[i] = cutDimDiff;
                    }
                }
                if (Start[i] + Count[i] != bigBox.Start[i] + bigBox.Count[i])
                {
                    size_t cutDimDiff = bigBox.Start[i] + bigBox.Count[i] - Start[i] - Count[i];
                    size_t cutDimSize = bigBox.size() / bigBox.Count[i] * cutDimDiff;
                    if (cutDimSize > maxCutDimSize)
                    {
                        maxCutDimSize = cutDimSize;
                        maxCutDimBox = bigBox;
                        maxCutDimBox.Start[i] = Start[i] + Count[i];
                        maxCutDimBox.Count[i] = cutDimDiff;
                    }
                }
            }
        }

        // cut the max cut dimension
        if (maxCutDimSize > 0)
        {
            regularBoxes.push_back(maxCutDimBox);
            QueryBox bigBoxRemained(bigBox);
            for (size_t i = 0; i < DimCount; ++i)
            {
                if (maxCutDimBox.Start[i] == bigBox.Start[i] &&
                    maxCutDimBox.Count[i] == bigBox.Count[i])
                {
                    continue;
                }
                else
                {
                    if (maxCutDimBox.Start[i] == bigBox.Start[i])
                    {
                        bigBoxRemained.Start[i] = maxCutDimBox.Start[i] + maxCutDimBox.Count[i];
                        bigBoxRemained.Count[i] =
                            bigBox.Start[i] + bigBox.Count[i] - bigBoxRemained.Start[i];
                    }
                    else
                    {
                        bigBoxRemained.Count[i] = maxCutDimBox.Start[i] - bigBox.Start[i];
                    }
                }
            }

            NdCut(bigBoxRemained, regularBoxes);
        }
    }

    void GetMaxInteractBox(const std::set<std::string> &samePrefixKeys, const size_t &max_depth,
                           size_t current_depth, std::vector<QueryBox> &regularBoxes,
                           std::vector<std::string> &cachedKeys)
    {
        if (current_depth > max_depth)
        {
            return;
        }
        current_depth++;
        QueryBox maxInteractBox(this->DimCount);
        std::string maxInteractKey;
        bool foundMaxInteract = false;
        for (auto &key : samePrefixKeys)
        {
            QueryBox const box(key);
            QueryBox intersection(this->DimCount);
            if (this->IsInteracted(box, intersection))
            {
                if (maxInteractBox.size() < intersection.size())
                {
                    maxInteractBox = intersection;
                    maxInteractKey = key;
                    foundMaxInteract = true;
                }
            }
        }

        if (!foundMaxInteract)
        {
            regularBoxes.push_back(*this);
            return;
        }

        cachedKeys.push_back(maxInteractKey);

        // If the current box is the max interact box, return, avoid cutting
        if (this->size() == maxInteractBox.size())
        {
            return;
        }

        if (current_depth == max_depth)
        {
            maxInteractBox.NdCut(*this, regularBoxes);
        }
        else
        {
            std::vector<QueryBox> nextBoxes;
            maxInteractBox.NdCut(*this, nextBoxes);
            for (auto &box : nextBoxes)
            {
                box.GetMaxInteractBox(samePrefixKeys, max_depth, current_depth, regularBoxes,
                                      cachedKeys);
            }
        }
    }
};

} // namespace kvcache
} // namespace adios2

#endif // ADIOS2_KVCACHE_QUERYBOX_H
