/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "CampaignIndexReader.h"
#include "adios2/core/IO.h"
#include "adios2/helper/adiosFunctions.h"

#include <nlohmann_json.hpp>
#include <sqlite3.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <numeric>
#include <sstream>
#include <type_traits>
#include <unordered_map>

namespace adios2
{
namespace core
{
namespace engine
{
namespace
{
using Database = std::unique_ptr<sqlite3, decltype(&sqlite3_close)>;

// Own statements even if conversion or variable definition throws. SQLite is
// opened read-only and held only while loading the catalog.
class Query
{
public:
    Query(sqlite3 *db, const char *sql) : m_DB(db)
    {
        if (sqlite3_prepare_v2(db, sql, -1, &m_Statement, nullptr) != SQLITE_OK)
        {
            sqlite3_finalize(m_Statement);
            throw std::runtime_error(std::string("CampaignIndex: ") + sqlite3_errmsg(db));
        }
    }
    ~Query() { sqlite3_finalize(m_Statement); }
    Query(const Query &) = delete;
    Query &operator=(const Query &) = delete;
    bool Next()
    {
        const int rc = sqlite3_step(m_Statement);
        if (rc == SQLITE_ROW)
            return true;
        if (rc != SQLITE_DONE)
            throw std::runtime_error(std::string("CampaignIndex: ") + sqlite3_errmsg(m_DB));
        return false;
    }
    std::string Text(int column) const
    {
        const auto p = sqlite3_column_text(m_Statement, column);
        return p ? std::string(reinterpret_cast<const char *>(p),
                               sqlite3_column_bytes(m_Statement, column))
                 : std::string();
    }
    int64_t Integer(int column) const { return sqlite3_column_int64(m_Statement, column); }

private:
    sqlite3 *m_DB;
    sqlite3_stmt *m_Statement = nullptr;
};

// ACX uses the VariableType enumeration in hpc_campaign/types.py. Resolve it
// through variabletypes instead of depending on ADIOS's enum numeric values.
DataType IndexType(std::string name)
{
    if (name == "long_double")
        name = "long double";
    if (name == "float_complex")
        name = "float complex";
    if (name == "double_complex")
        name = "double complex";
    return helper::GetDataTypeFromString(name);
}

template <class T>
T Number(const std::string &text)
{
    if constexpr (std::is_same<T, std::string>::value)
        return text;
    else if constexpr (std::is_integral<T>::value && std::is_signed<T>::value)
        return static_cast<T>(std::stoll(text));
    else if constexpr (std::is_integral<T>::value)
        return static_cast<T>(std::stoull(text));
    else if constexpr (std::is_floating_point<T>::value)
    {
        // strtold also accepts the nan/inf spellings in ADIOS statistics.
        char *end = nullptr;
        const long double value = std::strtold(text.c_str(), &end);
        if (end == text.c_str() || *end != '\0')
            throw std::runtime_error("CampaignIndex: invalid numeric metadata: " + text);
        return static_cast<T>(value);
    }
    else
    {
        T value;
        std::istringstream input(text);
        if (!(input >> value))
            throw std::runtime_error("CampaignIndex: invalid complex metadata: " + text);
        return value;
    }
}

template <class T>
T JSONValue(const nlohmann::json &value)
{
    if constexpr (std::is_arithmetic<T>::value || std::is_same<T, std::string>::value)
        return value.get<T>();
    else
    {
        if (value.is_string())
            return Number<T>(value.get<std::string>());
        return T(value.at(0).get<typename T::value_type>(),
                 value.at(1).get<typename T::value_type>());
    }
}

template <class T>
void DefineEntry(IO &io, Engine *engine, const std::string &name, const Dims &shape, size_t steps,
                 const std::string &minimum, const std::string &maximum, const std::string &value)
{
    auto &v = io.DefineVariable<T>(name, shape, Dims(shape.size(), 0), shape);
    v.m_Engine = engine;
    v.m_RandomAccess = engine->OpenMode() == Mode::ReadRandomAccess;
    v.m_AvailableStepsCount = steps;
    v.m_AvailableStepsStart = 0;
    // The index has one summary shape, rather than per-step shapes.
    if (!minimum.empty() && !maximum.empty())
    {
        v.m_Min = Number<T>(minimum);
        v.m_Max = Number<T>(maximum);
    }
    else if constexpr (std::is_arithmetic<T>::value)
    {
        MinMaxStruct unset;
        unset.Init(v.m_Type);
        std::memcpy(&v.m_Min, &unset.MinUnion, sizeof(T));
        std::memcpy(&v.m_Max, &unset.MaxUnion, sizeof(T));
    }
    if (v.m_SingleValue && steps == 1 && !value.empty())
    {
        auto j = nlohmann::json::parse(value);
        if (j.is_array() && j.size() == 1)
            j = nlohmann::json(j.at(0));
        if (!j.is_null())
            v.m_Value = JSONValue<T>(j);
    }
}

template <class T>
void DefineIndexAttribute(IO &io, const std::string &name, const std::string &text)
{
    const auto j = nlohmann::json::parse(text);
    if (j.is_array())
    {
        std::vector<T> values;
        values.reserve(j.size());
        for (const auto &item : j)
            values.push_back(JSONValue<T>(item));
        io.DefineAttribute<T>(name, values.data(), values.size());
    }
    else
        io.DefineAttribute<T>(name, JSONValue<T>(j));
}
} // namespace

CampaignIndexReader::CampaignIndexReader(IO &io, const std::string &name, Mode mode,
                                         helper::Comm comm)
: Engine("CampaignIndexReader", io, name, mode, std::move(comm))
{
    if (mode != Mode::Read && mode != Mode::ReadRandomAccess)
        throw std::invalid_argument("CampaignIndex only supports reading");
    Init();
    m_IsOpen = true;
}

void CampaignIndexReader::Init()
{
    m_IO.m_ReadStreaming = false;
    sqlite3 *raw = nullptr;
    const int rc = sqlite3_open_v2(m_Name.c_str(), &raw, SQLITE_OPEN_READONLY, nullptr);
    Database db(raw, sqlite3_close);
    if (rc != SQLITE_OK)
        throw std::runtime_error("CampaignIndex: cannot open " + m_Name + ": " +
                                 (raw ? sqlite3_errmsg(raw) : "SQLite allocation failed"));
    Query info(db.get(), "SELECT id FROM info");
    if (!info.Next() || info.Text(0) != "ACX")
        throw std::runtime_error("CampaignIndex: not an hpc-campaign index: " + m_Name);

    std::unordered_map<int64_t, DataType> types;
    Query type(db.get(), "SELECT typeid, name FROM variabletypes");
    while (type.Next())
        types.emplace(type.Integer(0), IndexType(type.Text(1)));

    std::unordered_map<int64_t, std::string> prefixes;
    Query datasets(db.get(), "SELECT d.datasetid, a.name, d.name FROM datasets d "
                             "JOIN archives a ON a.archiveid = d.archiveid");
    while (datasets.Next())
        prefixes.emplace(datasets.Integer(0), datasets.Text(1) + "/" + datasets.Text(2) + "/");

    Query vars(db.get(), "SELECT datasetid, name, type, steps, min, max, ndim, shape, value "
                         "FROM variables");
    while (vars.Next())
    {
        const std::string name = prefixes.at(vars.Integer(0)) + vars.Text(1);
        const auto type = types.at(vars.Integer(2));
        const auto stepCount = vars.Integer(3);
        if (stepCount < 1)
            throw std::runtime_error("CampaignIndex: invalid steps for " + name);
        const size_t steps = static_cast<size_t>(stepCount);
        Dims shape;
        std::istringstream dimensions(vars.Text(7));
        size_t extent;
        while (dimensions >> extent)
        {
            shape.push_back(extent);
            dimensions >> std::ws;
            if (dimensions.peek() == ',')
                dimensions.get();
        }
        if (vars.Integer(6) != static_cast<int64_t>(shape.size()))
            throw std::runtime_error("CampaignIndex: unsupported shape for " + name);
        bool supported = false;
#define declare_type(T)                                                                            \
    if (type == helper::GetDataType<T>())                                                          \
    {                                                                                              \
        DefineEntry<T>(m_IO, this, name, shape, steps, vars.Text(4), vars.Text(5), vars.Text(8));  \
        supported = true;                                                                          \
    }
        ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
        if (!supported)
            throw std::runtime_error("CampaignIndex: unsupported type for " + name);
        m_Steps = std::max(m_Steps, steps);
    }
    Query attrs(db.get(), "SELECT datasetid, name, type, value FROM attributes");
    while (attrs.Next())
    {
        const std::string name = prefixes.at(attrs.Integer(0)) + attrs.Text(1);
        const auto type = types.at(attrs.Integer(2));
        bool supported = false;
#define declare_type(T)                                                                            \
    if (type == helper::GetDataType<T>())                                                          \
    {                                                                                              \
        DefineIndexAttribute<T>(m_IO, name, attrs.Text(3));                                        \
        supported = true;                                                                          \
    }
        ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
        if (!supported)
            throw std::runtime_error("CampaignIndex: unsupported attribute type for " + name);
    }
}

StepStatus CampaignIndexReader::BeginStep(StepMode mode, float)
{
    if (m_OpenMode == Mode::ReadRandomAccess || mode != StepMode::Read || m_InStep)
        throw std::logic_error("CampaignIndex: invalid BeginStep call");
    if (m_CurrentStep >= m_Steps)
        return StepStatus::EndOfStream;
    m_IO.m_ReadStreaming = true;
    m_IO.m_EngineStep = m_CurrentStep;
    for (const auto &entry : m_IO.GetVariables())
    {
        entry.second->ResetStepsSelection(false);
        entry.second->m_StepPresent =
            m_CurrentStep < entry.second->m_AvailableStepsCount ? m_CurrentStep : MaxSizeT - 1;
    }
    m_InStep = true;
    return StepStatus::OK;
}
size_t CampaignIndexReader::CurrentStep() const { return m_CurrentStep; }
void CampaignIndexReader::EndStep()
{
    if (!m_InStep)
        throw std::logic_error("CampaignIndex: EndStep without BeginStep");
    m_InStep = false;
    ++m_CurrentStep;
}
void CampaignIndexReader::PerformGets() {}
void CampaignIndexReader::DoClose(int)
{
    m_InStep = false;
    m_IO.m_ReadStreaming = false;
}
size_t CampaignIndexReader::DoSteps() const { return m_Steps; }
void CampaignIndexReader::DoGetAbsoluteSteps(const VariableBase &v,
                                             std::vector<size_t> &steps) const
{
    steps.resize(v.m_AvailableStepsCount);
    std::iota(steps.begin(), steps.end(), 0);
}
bool CampaignIndexReader::VarShape(const VariableBase &v, size_t step, Dims &shape) const
{
    if (step != DefaultSizeT && step >= v.m_AvailableStepsCount)
        return false;
    shape = v.m_Shape;
    return true;
}
MinVarInfo *CampaignIndexReader::MinBlocksInfo(const VariableBase &v, size_t step) const
{
    if (step >= v.m_AvailableStepsCount)
        return nullptr;
    // Expose the summary shape, but no invented block decomposition.
    auto *info = new MinVarInfo(static_cast<int>(v.m_Shape.size()), v.m_Shape.data());
    info->Step = step;
    info->WasLocalValue = false;
    info->IsValue = v.m_SingleValue;
    return info;
}
bool CampaignIndexReader::VariableMinMax(const VariableBase &v, size_t step, MinMaxStruct &mm)
{
    mm.Init(v.m_Type);
    // Multi-step indexes contain aggregate extrema only.
    if (step != DefaultSizeT && (step >= v.m_AvailableStepsCount || v.m_AvailableStepsCount > 1))
        return true;
#define declare_type(T)                                                                            \
    if constexpr (std::is_arithmetic<T>::value)                                                    \
    {                                                                                              \
        if (v.m_Type == helper::GetDataType<T>())                                                  \
        {                                                                                          \
            const auto &variable = static_cast<const Variable<T> &>(v);                            \
            std::memcpy(&mm.MinUnion, &variable.m_Min, sizeof(T));                                 \
            std::memcpy(&mm.MaxUnion, &variable.m_Max, sizeof(T));                                 \
        }                                                                                          \
    }
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
    return true;
}

#define declare_type(T)                                                                            \
    void CampaignIndexReader::DoGetSync(Variable<T> &v, T *data)                                   \
    {                                                                                              \
        /* bpls -l requests scalar values. These are cached index metadata. */                     \
        if (!v.m_SingleValue || v.m_AvailableStepsCount != 1)                                      \
            throw std::runtime_error("CampaignIndex: variable data reads are not implemented");    \
        *data = v.m_Value;                                                                         \
    }                                                                                              \
    void CampaignIndexReader::DoGetDeferred(Variable<T> &v, T *data) { DoGetSync(v, data); }       \
    std::vector<typename Variable<T>::BPInfo> CampaignIndexReader::DoBlocksInfo(                   \
        const Variable<T> &, size_t) const                                                         \
    {                                                                                              \
        return {};                                                                                 \
    }                                                                                              \
    std::map<size_t, std::vector<typename Variable<T>::BPInfo>>                                    \
    CampaignIndexReader::DoAllStepsBlocksInfo(const Variable<T> &) const                           \
    {                                                                                              \
        return {};                                                                                 \
    }                                                                                              \
    std::vector<std::vector<typename Variable<T>::BPInfo>>                                         \
    CampaignIndexReader::DoAllRelativeStepsBlocksInfo(const Variable<T> &) const                   \
    {                                                                                              \
        return {};                                                                                 \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

} // namespace engine
} // namespace core
} // namespace adios2
