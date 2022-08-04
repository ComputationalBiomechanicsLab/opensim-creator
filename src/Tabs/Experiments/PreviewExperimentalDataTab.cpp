#include "PreviewExperimentalDataTab.hpp"

#include "src/Graphics/Renderer.hpp"
#include "src/Maths/BVH.hpp"
#include "src/Maths/Transform.hpp"
#include "src/Platform/Log.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/Assertions.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Widgets/LogViewerPanel.hpp"

#include <glm/vec4.hpp>
#include <IconsFontAwesome5.h>
#include <nonstd/span.hpp>
#include <OpenSim/Common/Storage.h>
#include <SDL_events.h>

#include <iostream>
#include <regex>
#include <string>
#include <utility>

namespace
{
    // describes the type of data held in a column of the data file
    enum class ColumnDataType {
        Point = 0,
        PointForce,
        BodyForce,
        Orientation,
        Unknown,
        TOTAL,
    };

    // human-readable name of a data type
    static auto const g_ColumnDataTypeStrings = osc::MakeArray<osc::CStringView, static_cast<int>(ColumnDataType::TOTAL)>
    (
        "Point",
        "PointForce",
        "BodyForce",
        "Orientation",
        "Unknown"
    );

    // the number of floating-point values the column is backed by
    static auto const g_ColumnDataSizes = osc::MakeArray<int, static_cast<int>(ColumnDataType::TOTAL)>
    (
        3,
        6,
        3,
        4,
        1
    );

    // prints a human-readable representation of a column data type
    std::ostream& operator<<(std::ostream& o, ColumnDataType dt)
    {
        return o << g_ColumnDataTypeStrings.at(static_cast<size_t>(dt));
    }

    // returns the number of floating-point values the column is backed by
    constexpr int NumElementsIn(ColumnDataType dt)
    {
        return g_ColumnDataSizes.at(static_cast<size_t>(dt));
    }

    // a struct that describes how a sequence of N column labels matches up to a column data type (with size N)
    struct ColumnDataTypeMatcher final {
        ColumnDataType Type;
        std::vector<osc::CStringView> Suffixes;

        ColumnDataTypeMatcher(ColumnDataType t, std::vector<osc::CStringView> suffixes) :
            Type{std::move(t)},
            Suffixes{std::move(suffixes)}
        {
            OSC_ASSERT(Suffixes.size() == NumElementsIn(Type));
            OSC_ASSERT(!Suffixes.empty());
        }
    };

    // a sequence of matchers to test against
    //
    // if the next N columns don't match any matchers, assume the column is `ColumnDataType::Unknown`
    static std::vector<ColumnDataTypeMatcher> const g_Matchers =
    {
        {ColumnDataType::PointForce, {"_vx", "_vy", "_vz", "_px", "_py", "_pz"}},
        {ColumnDataType::Point, {"_vx", "_vy", "_vz"}},
        {ColumnDataType::Point, {"_tx", "_ty", "_tz"}},
        {ColumnDataType::Point, {"_px", "_py", "_pz"}},
        {ColumnDataType::Orientation, {"_1", "_2", "_3", "_4"}},
        {ColumnDataType::Point, {"_1", "_2", "_3"}},
        {ColumnDataType::BodyForce, {"_fx", "_fy", "_fz"}},
    };

    // returns the number of columns the data type would require
    constexpr int NumColumnsRequiredBy(ColumnDataTypeMatcher const& matcher)
    {
        return NumElementsIn(matcher.Type);
    }

    // describes the layout of a single column parsed from the data file
    struct ColumnDescription final {

        int Offset;
        std::string Label;
        ColumnDataType DataType;

        ColumnDescription(int offset, std::string label, ColumnDataType dataType) :
            Offset{std::move(offset)},
            Label{std::move(label)},
            DataType{std::move(dataType)}
        {
        }
    };

    // prints a human-readable representation of a column description
    std::ostream& operator<<(std::ostream& o, ColumnDescription const& desc)
    {
        return o << "ColumnDescription(Offset=" << desc.Offset << ", DataType = " << desc.DataType << ", Label = \"" << desc.Label << "\")";
    }

    // returns true if `s` ends with the given `suffix`
    bool EndsWith(std::string const& s, std::string_view suffix)
    {
        if (suffix.length() > s.length())
        {
            return false;
        }

        return s.compare(s.length() - suffix.length(), suffix.length(), suffix) == 0;
    }

    // returns `true` if the provided labels [offset..offset+matcher.Suffixes.size()] all match up
    bool IsMatch(OpenSim::Array<std::string> const& labels, int offset, ColumnDataTypeMatcher const& matcher)
    {
        int colsRemaining = labels.size() - offset;
        int numColsToTest = NumColumnsRequiredBy(matcher);

        if (numColsToTest > colsRemaining)
        {
            return false;
        }

        for (int i = 0; i < numColsToTest; ++i)
        {
            std::string const& label = labels[offset + i];
            osc::CStringView requiredSuffix = matcher.Suffixes[i];

            if (!EndsWith(label, requiredSuffix))
            {
                return false;
            }
        }
        return true;
    }

    // returns the matching column data type for the next set of columns - if a match can be found
    std::optional<ColumnDataTypeMatcher> TryMatchColumnsWithType(OpenSim::Array<std::string> const& labels, int offset)
    {
        for (ColumnDataTypeMatcher const& matcher : g_Matchers)
        {
            if (IsMatch(labels, offset, matcher))
            {
                return matcher;
            }
        }
        return std::nullopt;
    }

    // returns a string that has had the 
    std::string RemoveLastNCharacters(std::string const& s, size_t n)
    {
        if (n > s.length())
        {
            return {};
        }

        return s.substr(0, s.length() - n);
    }

    // returns a sequence of parsed column descriptions, based on header labels
    std::vector<ColumnDescription> ParseColumnDescriptions(OpenSim::Array<std::string> const& labels)
    {
        std::vector<ColumnDescription> rv;
        int offset = 1;  // offset 0 == "time" (skip it)

        while (offset < labels.size())
        {
            if (std::optional<ColumnDataTypeMatcher> match = TryMatchColumnsWithType(labels, offset))
            {
                std::string baseName = RemoveLastNCharacters(labels[offset], match->Suffixes.front().size());

                rv.emplace_back(offset, baseName, match->Type);
                offset += NumElementsIn(match->Type);
            }
            else
            {
                rv.emplace_back(offset, labels[offset], ColumnDataType::Unknown);
                offset += 1;
            }
        }
        return rv;
    }

    // motions that were parsed from the file
    struct LoadedMotion final {
        std::vector<ColumnDescription> ColumnDescriptions;
        size_t RowStride = 0;
        std::vector<double> Data;
    };

    // prints `LoadedMotion` in a human-readable format
    std::ostream& operator<<(std::ostream& o, LoadedMotion const& mot)
    {
        o << "LoadedMotion(";
        o << "\n    ColumnDescriptions = [";
        for (ColumnDescription const& d : mot.ColumnDescriptions)
        {
            o << "\n        " << d;
        }
        o << "\n    ],";
        o << "\n    RowStride = " << mot.RowStride << ',';
        o << "\n    Data = [... " << mot.Data.size() << " values (" << mot.Data.size()/mot.RowStride << " rows)...]";
        o << "\n)";

        return o;
    }

    // get the time value for a given row
    double GetTime(LoadedMotion const& m, size_t row)
    {
        return m.Data.at(row * m.RowStride);
    }

    // get data values for a given row
    nonstd::span<double const> GetData(LoadedMotion const& m, size_t row)
    {
        OSC_ASSERT((row + 1) * m.RowStride <= m.Data.size());

        size_t start = (row * m.RowStride) + 1;
        size_t numValues = m.RowStride - 1;

        return nonstd::span<double const>(m.Data.data() + start, numValues);
    }

    // compute the stride of the data columns
    size_t CalcDataStride(nonstd::span<ColumnDescription const> descriptions)
    {
        size_t sum = 0;
        for (ColumnDescription const& d : descriptions)
        {
            sum += NumElementsIn(d.DataType);
        }
        return sum;
    }

    // compute the total row stride (time + data columns)
    size_t CalcRowStride(nonstd::span<ColumnDescription const> descriptions)
    {
        return 1 + CalcDataStride(descriptions);
    }

    // load raw row values from an OpenSim::Storage class
    std::vector<double> LoadRowValues(OpenSim::Storage const& storage, size_t rowStride)
    {
        size_t numDataCols = rowStride - 1;
        int numRows = storage.getSize();
        OSC_ASSERT(numRows > 0);

        std::vector<double> rv;
        rv.reserve(numRows * rowStride);

        // pack the data into the data vector
        for (int row = 0; row < storage.getSize(); ++row)
        {
            OpenSim::StateVector const& v = *storage.getStateVector(row);
            double t = v.getTime();
            OpenSim::Array<double> const& vs = v.getData();
            size_t numCols = std::min(static_cast<size_t>(v.getSize()), numDataCols);

            rv.push_back(t);
            for (size_t col = 0; col < numCols; ++col)
            {
                rv.push_back(vs[static_cast<int>(col)]);
            }
            for (size_t missing = numCols; missing < numDataCols; ++missing)
            {
                rv.push_back(0.0);  // pack any missing values
            }
        }
        OSC_ASSERT(rv.size() == numRows * rowStride);

        return rv;
    }

    // load motion from disk
    static LoadedMotion LoadData()
    {
        std::string const inputFileName = R"(E:\OneDrive\work_current\Gijs - IMU fitting\abduction_bad2.sto)";
        OpenSim::Storage const storage{inputFileName};

        LoadedMotion rv;
        rv.ColumnDescriptions = ParseColumnDescriptions(storage.getColumnLabels());
        rv.RowStride = CalcRowStride(rv.ColumnDescriptions);
        rv.Data = LoadRowValues(storage, rv.RowStride);
        return rv;
    }
}

class osc::PreviewExperimentalDataTab::Impl final {
public:
    Impl(TabHost* parent) : m_Parent{std::move(parent)}
    {
        osc::log::info("%s", StreamToString(m_Motion).c_str());
    }

    UID getID() const
    {
        return m_ID;
    }

    CStringView getName() const
    {
        return m_Name;
    }

    TabHost* parent()
    {
        return m_Parent;
    }

    void onMount()
    {

    }

    void onUnmount()
    {

    }

    bool onEvent(SDL_Event const&)
    {
        return false;
    }

    void onTick()
    {

    }

    void onDrawMainMenu()
    {

    }

    void onDraw()
    {
        m_LogViewer.draw();
    }


private:
    UID m_ID;
    std::string m_Name = ICON_FA_DOT_CIRCLE " Experimental Data";
    TabHost* m_Parent;

    LogViewerPanel m_LogViewer{"Log"};
    LoadedMotion m_Motion = LoadData();
};


// public API

osc::PreviewExperimentalDataTab::PreviewExperimentalDataTab(TabHost* parent) :
    m_Impl{new Impl{std::move(parent)}}
{
}

osc::PreviewExperimentalDataTab::PreviewExperimentalDataTab(PreviewExperimentalDataTab&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::PreviewExperimentalDataTab& osc::PreviewExperimentalDataTab::operator=(PreviewExperimentalDataTab&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::PreviewExperimentalDataTab::~PreviewExperimentalDataTab() noexcept
{
    delete m_Impl;
}

osc::UID osc::PreviewExperimentalDataTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::PreviewExperimentalDataTab::implGetName() const
{
    return m_Impl->getName();
}

osc::TabHost* osc::PreviewExperimentalDataTab::implParent() const
{
    return m_Impl->parent();
}

void osc::PreviewExperimentalDataTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::PreviewExperimentalDataTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::PreviewExperimentalDataTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::PreviewExperimentalDataTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::PreviewExperimentalDataTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::PreviewExperimentalDataTab::implOnDraw()
{
    m_Impl->onDraw();
}
