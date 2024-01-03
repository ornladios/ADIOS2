//
// Created by cguo51 on 7/27/23.
//

#ifndef ADIOS2_KVCACHE_QUERYBOX_H
#define ADIOS2_KVCACHE_QUERYBOX_H

#include <set>
#include <iostream>
#include <adios2/common/ADIOSTypes.h>
#include <nlohmann_json.hpp>

namespace adios2
{
// QueryBox is a class to represent a query box in a multi-dimensional space
class QueryBox
{
public:
    adios2::Dims start{};
    adios2::Dims count{};

    // constructor
    QueryBox() = default;
    QueryBox(const adios2::Dims &start, const adios2::Dims &count) : start(start), count(count){};
    QueryBox(const std::string &key){
        // sample key: "U3218446744073709551615__count_:_64_64_64___start_:_0_0_0__", count [64, 64, 64], start [0, 0, 0]
        // using Dims = std::vector<size_t>;
        auto lf_ExtractDimensions = [](const std::string &key, const std::string &delimiter) -> Dims {
            size_t const pos = key.find(delimiter);
            size_t const end = key.find("__", pos + delimiter.length());
            std::string dimStr = key.substr(pos + delimiter.length(), end - pos - delimiter.length());
            Dims dimensions;
            std::istringstream dimStream(dimStr);
            std::string token;
            while (std::getline(dimStream, token, '_')) {
                dimensions.push_back(std::stoul(token));
            }
            return dimensions;
        };

        this->start = lf_ExtractDimensions(key, "__start_:_");
        this->count = lf_ExtractDimensions(key, "__count_:_");
    }

    // size
    size_t size() const
    {
        size_t s = 1;
        for (auto &d : count)
        {
            s *= d;
        }
        return s;
    }

    // Serialize QueryBox to a string, like __count_:_64_64_64___start_:_0_0_0__
    static std::string serializeQueryBox(const QueryBox &box)
    {
        nlohmann::json jsonBox;
        jsonBox["start"] = box.start;
        jsonBox["count"] = box.count;
        return jsonBox.dump();
    }

    // determine if a query box is equal to another query box
    bool operator==(const QueryBox &box) const
    {
        return start == box.start && count == box.count;
    }

    // determine if a query box is interacted in another query box, return intersection part as a new query box
    bool isInteracted (const QueryBox &box, QueryBox &intersection) const
    {
        if (start.size() != box.start.size() || start.size() != count.size() ||
            start.size() != box.count.size())
        {
            return false;
        }
        for (size_t i = 0; i < start.size(); ++i)
        {
            if (start[i] > box.start[i] + box.count[i] || box.start[i] > start[i] + count[i])
            {
                return false;
            }
        }
        intersection.start.resize(start.size());
        intersection.count.resize(count.size());
        for (size_t i = 0; i < start.size(); ++i)
        {
            intersection.start[i] = std::max(start[i], box.start[i]);
            intersection.count[i] =
                std::min(start[i] + count[i], box.start[i] + box.count[i]) - intersection.start[i];
        }
        return true;
    }

    // determine if a query box is fully contained in another query box
    bool isFullContainedBy(const QueryBox &box)
    {
        if (start.size() != box.start.size() || start.size() != count.size() ||
            start.size() != box.count.size())
        {
            return false;
        }
        for (size_t i = 0; i < start.size(); ++i)
        {
            if (start[i] < box.start[i] || start[i] + count[i] > box.start[i] + box.count[i])
            {
                return false;
            }
        }
        return true;
    }

    // cut a query box from another interaction box, return a list of regular box
    // remainingBox is the big one, this is small one
    void interactionCut(const QueryBox &remainingBox, std::vector<QueryBox> &regularBoxes)
    {
        if (remainingBox == *this)
        {
            return;
        }

        // find the max cut dimension
        size_t maxCutDimSize = 0;
        QueryBox maxCutDimBox;
        for (size_t i = 0; i < start.size(); ++i)
        {
            if (start[i] == remainingBox.start[i] && count[i] == remainingBox.count[i])
            {
                continue;
            }
            else {
                if (start[i] != remainingBox.start[i]){
                    size_t cutDimDiff = start[i] - remainingBox.start[i];
                    size_t cutDimSize = remainingBox.size() / remainingBox.count[i] * cutDimDiff;
                    if (cutDimSize > maxCutDimSize)
                    {
                        maxCutDimSize = cutDimSize;
                        maxCutDimBox = QueryBox(remainingBox.start, remainingBox.count);
                        maxCutDimBox.count[i] = cutDimDiff;
                    }
                }

                if (start[i] + count[i] != remainingBox.start[i] + remainingBox.count[i]){
                    size_t cutDimDiff = remainingBox.start[i] + remainingBox.count[i] - start[i] - count[i];
                    size_t cutDimSize = remainingBox.size() / count[i] * cutDimDiff;
                    if (cutDimSize > maxCutDimSize)
                    {
                        maxCutDimSize = cutDimSize;
                        maxCutDimBox = QueryBox(remainingBox.start, remainingBox.count);
                        maxCutDimBox.start[i] = start[i] + count[i];
                        maxCutDimBox.count[i] = cutDimDiff;
                    }
                }
            }
        }

        // cut the max cut dimension
        if (maxCutDimSize > 0)
        {
            regularBoxes.push_back(maxCutDimBox);
            QueryBox remainingBox1 = QueryBox(remainingBox.start, remainingBox.count);
            for (size_t i = 0; i < remainingBox.start.size(); ++i)
            {
                if (maxCutDimBox.start[i] == remainingBox.start[i] && maxCutDimBox.count[i] == remainingBox.count[i])
                {
                    continue;
                }
                else {
                    if (maxCutDimBox.start[i] != remainingBox.start[i])
                    {
                        remainingBox1.count[i] = maxCutDimBox.start[i] - remainingBox.start[i];
                    } else {
                        remainingBox1.start[i] = maxCutDimBox.start[i] + maxCutDimBox.count[i];
                        remainingBox1.count[i] = remainingBox.start[i] + remainingBox.count[i] - remainingBox1.start[i];
                    }

                }
            }
            interactionCut(remainingBox1, regularBoxes);
        }
    }

    void getMaxInteractBox(const std::set<std::string> &samePrefixKeys, const size_t &max_depth, size_t current_depth, std::vector<QueryBox> &regularBoxes, std::vector<QueryBox> &cachedBox, std::vector<std::string> &cachedKeys)
    {
        if (current_depth > max_depth)
        {
            return;
        }
        current_depth++;
        QueryBox maxInteractBox;
        std::string maxInteractKey;
        for (auto &key : samePrefixKeys)
        {
            // std::cout << "Same Prefix Keys: " << key << " Current Depth: " << current_depth << std::endl;
            QueryBox const box(key);
            QueryBox intersection;
            if (this->isInteracted(box, intersection))
            {
                if (maxInteractBox.size() < intersection.size())
                {
                    maxInteractBox = intersection;
                    maxInteractKey = key;
                }
            }
        }

        if (maxInteractBox.count.size() == 0)
        {
            return;
        }

        // std::cout << "===============================================================================" << std::endl;
        // std::cout << "Current Depth: " << current_depth << " Target box: " << this->toString() << " Target box size: " << this->size() << std::endl;
        // std::cout << "Pushing maxInteractBox: " << maxInteractBox.toString() << " key: " << maxInteractKey << ", Interacted size: " << maxInteractBox.size() << std::endl;

        cachedBox.push_back(maxInteractBox);
        cachedKeys.push_back(maxInteractKey);

        if (current_depth == max_depth)
        {
            maxInteractBox.interactionCut(*this, regularBoxes);
        } else {
            std::vector<QueryBox> nextBoxes;
            maxInteractBox.interactionCut(*this, nextBoxes);
            for (auto &box : nextBoxes)
            {
                box.getMaxInteractBox(samePrefixKeys, max_depth, current_depth, regularBoxes, cachedBox, cachedKeys);
            }
        }   
    }

    // rewrite toString
    std::string toString() const
    {
        std::string str = "Box start: [";
        for (size_t i = 0; i < start.size(); ++i)
        {
            str += std::to_string(start[i]);
            if (i != start.size() - 1)
            {
                str += ", ";
            }
        }
        str += "], count: [";
        for (size_t i = 0; i < count.size(); ++i)
        {
            str += std::to_string(count[i]);
            if (i != count.size() - 1)
            {
                str += ", ";
            }
        }
        str += "]";
        return str;
    }
    
};
};
#endif // UNITTEST_QUERYBOX_H
